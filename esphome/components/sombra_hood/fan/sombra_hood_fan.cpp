#include "sombra_hood_fan.h"

#include "esphome/core/log.h"

namespace esphome::sombra_hood {

static const char *const TAG = "sombra_hood.fan";

void SombraHoodFan::setup() {
  // Speed used when the fan is switched on without an explicit speed.
  this->speed = 1;
}

void SombraHoodFan::dump_config() { LOG_FAN("", "Gutmann Sombra fan", this); }

void SombraHoodFan::control(const fan::FanCall &call) {
  const uint8_t current = this->parent_->get_speed();
  const bool on = call.get_state().value_or(this->state);
  uint8_t target = 0;
  if (on) {
    target = call.get_speed().value_or(this->speed);
    if (target < 1 || target > 4)
      target = 1;
  }

  if (target == current)
    return;
  if (target == 0) {
    // Pressing the key of the active speed switches the fan off.
    this->parent_->press(current);
  } else {
    this->parent_->press(target);
  }
}

void SombraHoodFan::on_leds_updated(bool force) {
  const uint8_t speed = this->parent_->get_speed();
  const bool state = speed != 0;
  if (state == this->state && (!state || speed == this->speed))
    return;
  this->state = state;
  if (state)
    this->speed = speed;
  this->publish_state();
}

}  // namespace esphome::sombra_hood
