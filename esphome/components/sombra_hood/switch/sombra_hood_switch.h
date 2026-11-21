#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/helpers.h"
#include "../sombra_hood.h"

namespace esphome::sombra_hood {

// Run-on timer of the hood. The state follows the LED; switching only presses
// the key and is confirmed once the hood has reacted.
class SombraHoodTimerSwitch : public switch_::Switch, public SombraHoodListener, public Parented<SombraHood> {
 public:
  void on_leds_updated(bool force) override {
    const bool on = this->parent_->get_led(KEY_TIMER);
    if (!this->published_ || on != this->state) {
      this->published_ = true;
      this->publish_state(on);
    }
  }

 protected:
  void write_state(bool state) override {
    if (state != this->parent_->get_led(KEY_TIMER))
      this->parent_->press(KEY_TIMER);
  }

  bool published_{false};
};

}  // namespace esphome::sombra_hood
