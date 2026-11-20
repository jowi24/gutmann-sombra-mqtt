#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/helpers.h"
#include "../sombra_hood.h"

namespace esphome::sombra_hood {

// Resets the filter cleaning indicator by holding the maintenance key for 6 s.
// Only pressed while the indicator is active, so the key's other function is
// never triggered by accident.
class SombraHoodFilterResetButton : public button::Button, public Parented<SombraHood> {
 protected:
  void press_action() override {
    if (this->parent_->get_led(KEY_MAINTENANCE))
      this->parent_->press(KEY_MAINTENANCE, PRESS_ROUNDS_LONG);
  }
};

}  // namespace esphome::sombra_hood
