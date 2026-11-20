#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/helpers.h"
#include "../sombra_hood.h"

namespace esphome::sombra_hood {

// Filter cleaning indicator (the maintenance LED blinks while it is active).
class SombraHoodMaintenanceSensor : public binary_sensor::BinarySensor,
                                    public SombraHoodListener,
                                    public Parented<SombraHood> {
 public:
  void on_leds_updated(bool force) override {
    const bool on = this->parent_->get_led(KEY_MAINTENANCE);
    if (!this->has_state() || on != this->state)
      this->publish_state(on);
  }
};

}  // namespace esphome::sombra_hood
