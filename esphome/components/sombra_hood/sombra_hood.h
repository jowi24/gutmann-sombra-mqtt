#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#ifdef USE_OTA_STATE_LISTENER
#include "esphome/components/ota/ota_backend.h"
#endif

namespace esphome::sombra_hood {

// Key/LED ids on the front panel. The same id addresses a key and its LED.
enum Key : uint8_t {
  KEY_SPEED_1 = 1,
  KEY_SPEED_2 = 2,
  KEY_SPEED_3 = 3,
  KEY_SPEED_4 = 4,
  KEY_TIMER = 5,
  KEY_LIGHT = 6,
  KEY_MAINTENANCE = 7,
};

static constexpr uint8_t NUM_KEYS = 8;  // index 0 unused

// Scan rounds (4 ms each) a key is held. 100 rounds = 400 ms, 1500 = 6 s.
static constexpr uint16_t PRESS_ROUNDS_SHORT = 100;
static constexpr uint16_t PRESS_ROUNDS_LONG = 1500;

class SombraHoodListener {
 public:
  // Called from loop() when the debounced LED states changed (force = false)
  // or periodically to reconcile the entity state with the LEDs (force = true).
  virtual void on_leds_updated(bool force) = 0;
};

class SombraHood : public Component
#ifdef USE_OTA_STATE_LISTENER
    ,
                   public ota::OTAGlobalStateListener
#endif
{
 public:
  void set_scan_pin(InternalGPIOPin *pin) { this->scan_pin_ = pin; }
  void set_led_pins(InternalGPIOPin *pin1, InternalGPIOPin *pin2) {
    this->led_pin_1_ = pin1;
    this->led_pin_2_ = pin2;
  }
  void set_key_pins(InternalGPIOPin *pin1, InternalGPIOPin *pin2) {
    this->key_pin_1_ = pin1;
    this->key_pin_2_ = pin2;
  }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

#ifdef USE_OTA_STATE_LISTENER
  void on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) override;
#endif

  void register_listener(SombraHoodListener *listener) { this->listeners_.push_back(listener); }

  // True while the SCAN signal is present and the LED states are trustworthy.
  bool is_synced() const { return this->synced_; }
  bool get_led(uint8_t key) const { return this->led_states_[key]; }
  // Active fan speed 1-4 according to the LEDs, 0 = off.
  uint8_t get_speed() const;

  // Simulates a key press for the given number of scan rounds. Ignored while
  // not synced, because the decision to press may be based on stale LEDs.
  void press(uint8_t key, uint16_t rounds = PRESS_ROUNDS_SHORT);

  // Stops the timer and the SCAN interrupt, e.g. during OTA updates.
  void pause();
  void resume();

 protected:
  static void scan_isr(SombraHood *arg);
  static void timer_isr();
  void on_timer_();
  void check_keys_(uint8_t line);
  void check_leds_(uint8_t line);
  void update_key_(uint8_t key, ISRInternalGPIOPin &pin);
  void update_led_counter_(uint8_t key, bool level);
  void release_keys_();
  void notify_listeners_(bool force);

  static SombraHood *instance;

  InternalGPIOPin *scan_pin_{nullptr};
  InternalGPIOPin *led_pin_1_{nullptr};
  InternalGPIOPin *led_pin_2_{nullptr};
  InternalGPIOPin *key_pin_1_{nullptr};
  InternalGPIOPin *key_pin_2_{nullptr};
  ISRInternalGPIOPin isr_led_pin_1_;
  ISRInternalGPIOPin isr_led_pin_2_;
  ISRInternalGPIOPin isr_key_pin_1_;
  ISRInternalGPIOPin isr_key_pin_2_;

  // Shared with the ISRs
  volatile bool led_states_[NUM_KEYS]{};
  volatile uint16_t led_counter_[NUM_KEYS]{};
  volatile uint16_t key_counter_[NUM_KEYS]{};
  volatile uint32_t phase_{0};
  volatile uint8_t resync_{0};
  volatile uint32_t last_scan_us_{0};
  volatile uint32_t sync_count_{0};
  volatile bool paused_{false};

  uint32_t last_sync_count_{0};
  uint32_t last_sync_ms_{0};
  uint32_t synced_since_ms_{0};
  uint32_t last_press_ms_{0};
  uint32_t last_reconcile_ms_{0};
  bool synced_{false};

  std::vector<SombraHoodListener *> listeners_;
};

}  // namespace esphome::sombra_hood
