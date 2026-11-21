#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/core/helpers.h"
#include "../sombra_hood.h"

namespace esphome::sombra_hood {

class SombraHoodLight : public light::LightOutput, public SombraHoodListener, public Parented<SombraHood> {
 public:
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::ON_OFF});
    return traits;
  }
  void setup_state(light::LightState *state) override { this->state_ = state; }
  void write_state(light::LightState *state) override {
    bool on;
    state->current_values_as_binary(&on);
    // Also called when on_leds_updated() syncs the light state; such a
    // request matches the LED and is dropped without pressing.
    this->parent_->request_state(KEY_LIGHT, on);
  }
  void on_leds_updated(bool force) override {
    if (this->state_ == nullptr || this->parent_->has_pending(KEY_LIGHT))
      return;
    const bool on = this->parent_->get_led(KEY_LIGHT);
    if (this->state_->remote_values.is_on() == on)
      return;
    auto call = on ? this->state_->turn_on() : this->state_->turn_off();
    call.perform();
  }

 protected:
  light::LightState *state_{nullptr};
};

}  // namespace esphome::sombra_hood
