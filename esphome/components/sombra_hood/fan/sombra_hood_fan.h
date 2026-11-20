#pragma once

#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "../sombra_hood.h"

namespace esphome::sombra_hood {

// Fan with the four speeds of the hood. The state follows the LEDs; commands
// only press keys and are confirmed once the hood has switched.
class SombraHoodFan : public Component, public fan::Fan, public SombraHoodListener, public Parented<SombraHood> {
 public:
  void setup() override;
  void dump_config() override;
  fan::FanTraits get_traits() override { return fan::FanTraits(false, true, false, 4); }
  void on_leds_updated(bool force) override;

 protected:
  void control(const fan::FanCall &call) override;
};

}  // namespace esphome::sombra_hood
