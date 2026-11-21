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
  const bool on = call.get_state().value_or(this->state);
  uint8_t target = 0;
  if (on) {
    target = call.get_speed().value_or(this->speed);
    if (target < 1 || target > 4)
      target = 1;
  }
  this->parent_->request_speed(target);
}

void SombraHoodFan::on_leds_updated(bool force) {
  if (this->parent_->has_pending_speed())
    return;
  const uint8_t speed = this->parent_->get_speed();
  const bool state = speed != 0;
  if (this->published_ && state == this->state && (!state || speed == this->speed))
    return;
  this->published_ = true;
  this->state = state;
  if (state)
    this->speed = speed;
  this->publish_state();
}

}  // namespace esphome::sombra_hood
