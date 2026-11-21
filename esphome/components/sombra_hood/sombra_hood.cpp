#include "sombra_hood.h"

#include <Arduino.h>

#include "esphome/core/log.h"

namespace esphome::sombra_hood {

static const char *const TAG = "sombra_hood";

// timer1 runs at 80 MHz / 16 = 5 ticks per µs, independent of the CPU clock.
static constexpr uint32_t TIMER_TICKS_100US = 500;
// One scan round takes 4 ms; accept some jitter when (re)synchronising.
static constexpr uint32_t SCAN_PERIOD_MIN_US = 3900;
static constexpr uint32_t SCAN_PERIOD_MAX_US = 4100;
// Resync roughly every 12 SCAN edges (see scan_isr).
static constexpr uint8_t RESYNC_EDGES = 10;
// A resync normally happens every ~50 ms. Without one the hood is off or the
// SCAN line is broken.
static constexpr uint32_t SYNC_TIMEOUT_MS = 500;
// Wait for the LED debounce to settle after (re)gaining sync.
static constexpr uint32_t SYNC_SETTLE_MS = 500;
// Debounce in scan rounds. The maintenance LED blinks, so switching it off
// needs a much longer stable phase.
static constexpr uint16_t LED_DEBOUNCE_ROUNDS = 20;
static constexpr uint16_t MAINTENANCE_OFF_ROUNDS = 375;
// Reconcile entity states with the LEDs, but leave some time after a key
// press for the hood to react.
static constexpr uint32_t RECONCILE_INTERVAL_MS = 2000;
static constexpr uint32_t PRESS_SETTLE_MS = 1500;
// When the hood switches speed on its own (run-on timer), the old LED goes off
// a few ms before the new one comes on. Wait for the LEDs to settle so that
// no short "off" is reported in between.
static constexpr uint32_t LED_SETTLE_MS = 200;

SombraHood *SombraHood::instance = nullptr;

void SombraHood::setup() {
  instance = this;

  this->scan_pin_->setup();
  this->led_pin_1_->setup();
  this->led_pin_2_->setup();
  this->key_pin_1_->setup();
  this->key_pin_2_->setup();

  // U3 (74HCT08) sits between ESP and hood; a floating HCT input would press
  // keys at random, so the key pins are always driven.
  this->key_pin_1_->digital_write(false);
  this->key_pin_2_->digital_write(false);

  this->isr_led_pin_1_ = this->led_pin_1_->to_isr();
  this->isr_led_pin_2_ = this->led_pin_2_->to_isr();
  this->isr_key_pin_1_ = this->key_pin_1_->to_isr();
  this->isr_key_pin_2_ = this->key_pin_2_->to_isr();

  timer1_isr_init();
  timer1_attachInterrupt(&SombraHood::timer_isr);
  // The timer is started by the first successful resync in scan_isr().
  this->scan_pin_->attach_interrupt(&SombraHood::scan_isr, this, gpio::INTERRUPT_RISING_EDGE);

#ifdef USE_OTA_STATE_LISTENER
  ota::get_global_ota_callback()->add_global_state_listener(this);
#endif
}

void SombraHood::dump_config() {
  ESP_LOGCONFIG(TAG, "Gutmann Sombra hood:");
  LOG_PIN("  SCAN Pin: ", this->scan_pin_);
  LOG_PIN("  LED Pin 1: ", this->led_pin_1_);
  LOG_PIN("  LED Pin 2: ", this->led_pin_2_);
  LOG_PIN("  Key Pin 1: ", this->key_pin_1_);
  LOG_PIN("  Key Pin 2: ", this->key_pin_2_);
}

void SombraHood::loop() {
  const uint32_t now = millis();

  const uint32_t sync_count = this->sync_count_;
  if (sync_count != this->last_sync_count_) {
    this->last_sync_count_ = sync_count;
    this->last_sync_ms_ = now;
  }
  const bool signal = this->last_sync_count_ != 0 && !this->paused_ && now - this->last_sync_ms_ < SYNC_TIMEOUT_MS;
  if (!signal) {
    if (this->synced_ || this->synced_since_ms_ != 0) {
      ESP_LOGW(TAG, "Lost SCAN signal from the hood");
    }
    this->synced_ = false;
    this->synced_since_ms_ = 0;
  } else if (!this->synced_) {
    if (this->synced_since_ms_ == 0) {
      this->synced_since_ms_ = now;
    } else if (now - this->synced_since_ms_ >= SYNC_SETTLE_MS) {
      ESP_LOGI(TAG, "Synchronised with the hood");
      this->synced_ = true;
      this->last_reconcile_ms_ = now;
      this->notify_listeners_(true);
    }
  }

  bool changed = false;
  for (uint8_t i = 1; i < NUM_KEYS; i++) {
    const uint16_t threshold =
        (i == KEY_MAINTENANCE && this->led_states_[i]) ? MAINTENANCE_OFF_ROUNDS : LED_DEBOUNCE_ROUNDS;
    if (this->led_counter_[i] > threshold) {
      this->led_states_[i] = !this->led_states_[i];
      this->led_counter_[i] = 0;
      changed = true;
    }
  }
  if (changed)
    this->led_changed_ms_ = now;

  if (!this->synced_)
    return;

  if (this->led_changed_ms_ != 0 && now - this->led_changed_ms_ >= LED_SETTLE_MS) {
    this->led_changed_ms_ = 0;
    ESP_LOGD(TAG, "LEDs: speed=%u timer=%s light=%s maintenance=%s", this->get_speed(),
             ONOFF(this->led_states_[KEY_TIMER]), ONOFF(this->led_states_[KEY_LIGHT]),
             ONOFF(this->led_states_[KEY_MAINTENANCE]));
    this->notify_listeners_(false);
  }

  bool pressing = false;
  for (uint8_t i = 1; i < NUM_KEYS; i++)
    pressing |= this->key_counter_[i] != 0;
  if (pressing)
    this->last_press_ms_ = now;

  if (!pressing && this->led_changed_ms_ == 0 && now - this->last_press_ms_ >= PRESS_SETTLE_MS &&
      now - this->last_reconcile_ms_ >= RECONCILE_INTERVAL_MS) {
    this->last_reconcile_ms_ = now;
    this->notify_listeners_(true);
  }
}

void SombraHood::notify_listeners_(bool force) {
  for (auto *listener : this->listeners_)
    listener->on_leds_updated(force);
}

uint8_t SombraHood::get_speed() const {
  for (uint8_t i = KEY_SPEED_1; i <= KEY_SPEED_4; i++) {
    if (this->led_states_[i])
      return i;
  }
  return 0;
}

void SombraHood::press(uint8_t key, uint16_t rounds) {
  if (key == 0 || key >= NUM_KEYS)
    return;
  if (!this->synced_) {
    ESP_LOGW(TAG, "Not synchronised with the hood, ignoring key %u", key);
    return;
  }
  ESP_LOGD(TAG, "Pressing key %u for %u ms", key, rounds * 4);
  this->key_counter_[key] = rounds;
  this->last_press_ms_ = millis();
}

void SombraHood::pause() {
  ESP_LOGI(TAG, "Pausing");
  this->paused_ = true;
  this->scan_pin_->detach_interrupt();
  timer1_disable();
  for (auto &counter : this->key_counter_)
    counter = 0;
  this->release_keys_();
}

void SombraHood::resume() {
  ESP_LOGI(TAG, "Resuming");
  this->resync_ = 0;
  this->paused_ = false;
  this->scan_pin_->attach_interrupt(&SombraHood::scan_isr, this, gpio::INTERRUPT_RISING_EDGE);
}

#ifdef USE_OTA_STATE_LISTENER
// The 100 µs timer interrupt disturbs the OTA transfer, so stop it while an
// update is running. On success the device reboots anyway.
void SombraHood::on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) {
  if (state == ota::OTA_STARTED) {
    this->pause();
  } else if ((state == ota::OTA_ERROR || state == ota::OTA_ABORT) && this->paused_) {
    this->resume();
  }
}
#endif

// ---- ISRs ----

void IRAM_ATTR SombraHood::scan_isr(SombraHood *arg) {
  if (arg->paused_)
    return;
  if (++arg->resync_ > RESYNC_EDGES) {
    const uint32_t now = micros();
    const uint32_t period = now - arg->last_scan_us_;
    if (period > SCAN_PERIOD_MIN_US && period < SCAN_PERIOD_MAX_US) {
      arg->phase_ = 1;
      arg->resync_ = 0;
      arg->sync_count_ = arg->sync_count_ + 1;
      timer1_disable();
      timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
      timer1_write(TIMER_TICKS_100US);
    }
    arg->last_scan_us_ = now;
  }
}

void IRAM_ATTR SombraHood::timer_isr() { instance->on_timer_(); }

// Called every 100 µs. Phase 0 is the rising edge of the SCAN line; each of the
// four scan lines is active for 10 phases (1 ms), in the order 1, 2, 4, 3.
void IRAM_ATTR SombraHood::on_timer_() {
  switch (this->phase_ % 40) {
    case 1:
      this->check_keys_(1);
      break;
    case 5:
      this->check_leds_(1);
      break;
    case 11:
      this->check_keys_(2);
      break;
    case 15:
      this->check_leds_(2);
      break;
    case 21:
      this->check_keys_(4);
      break;
    case 25:
      this->check_leds_(4);
      break;
    case 31:
      this->check_keys_(3);
      break;
    case 35:
      this->check_leds_(3);
      break;
    case 9:
    case 19:
    case 29:
    case 39:
      // The diode behind U3 blocks LOW, so the hood's line is left alone.
      this->release_keys_();
      break;
  }
  this->phase_ = this->phase_ + 1;
}

void IRAM_ATTR SombraHood::release_keys_() {
  this->isr_key_pin_1_.digital_write(false);
  this->isr_key_pin_2_.digital_write(false);
}

void IRAM_ATTR SombraHood::check_keys_(uint8_t line) {
  switch (line) {
    case 1:
      this->update_key_(KEY_SPEED_2, this->isr_key_pin_1_);
      this->update_key_(KEY_LIGHT, this->isr_key_pin_2_);
      break;
    case 2:
      this->update_key_(KEY_SPEED_3, this->isr_key_pin_1_);
      this->update_key_(KEY_MAINTENANCE, this->isr_key_pin_2_);
      break;
    case 3:
      this->update_key_(KEY_SPEED_1, this->isr_key_pin_1_);
      this->update_key_(KEY_TIMER, this->isr_key_pin_2_);
      break;
    case 4:
      this->update_key_(KEY_SPEED_4, this->isr_key_pin_1_);
      break;
  }
}

void IRAM_ATTR SombraHood::update_key_(uint8_t key, ISRInternalGPIOPin &pin) {
  if (this->key_counter_[key] > 0) {
    pin.digital_write(true);
    this->key_counter_[key] = this->key_counter_[key] - 1;
  }
}

void IRAM_ATTR SombraHood::check_leds_(uint8_t line) {
  const bool s1 = this->isr_led_pin_1_.digital_read();
  const bool s2 = this->isr_led_pin_2_.digital_read();
  switch (line) {
    case 1:
      this->update_led_counter_(KEY_SPEED_2, s1);
      this->update_led_counter_(KEY_LIGHT, s2);
      break;
    case 2:
      this->update_led_counter_(KEY_SPEED_3, s1);
      this->update_led_counter_(KEY_MAINTENANCE, s2);
      break;
    case 3:
      this->update_led_counter_(KEY_SPEED_1, s1);
      this->update_led_counter_(KEY_TIMER, s2);
      break;
    case 4:
      this->update_led_counter_(KEY_SPEED_4, s1);
      break;
  }
}

// Counts how long the line level has equalled the stored state; loop() flips
// the state once the count exceeds the debounce threshold. The stored state
// therefore settles at the inverted line level (ported unchanged from the
// Arduino firmware).
void IRAM_ATTR SombraHood::update_led_counter_(uint8_t key, bool level) {
  if (this->led_states_[key] == level) {
    this->led_counter_[key] = this->led_counter_[key] + 1;
  } else {
    this->led_counter_[key] = 0;
  }
}

}  // namespace esphome::sombra_hood
