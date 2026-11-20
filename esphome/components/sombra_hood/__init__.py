from esphome import pins
import esphome.codegen as cg
from esphome.components import ota
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE

CODEOWNERS = ["@jowi24"]
MULTI_CONF = False

CONF_SOMBRA_HOOD_ID = "sombra_hood_id"
CONF_SCAN_PIN = "scan_pin"
CONF_LED_PIN_1 = "led_pin_1"
CONF_LED_PIN_2 = "led_pin_2"
CONF_KEY_PIN_1 = "key_pin_1"
CONF_KEY_PIN_2 = "key_pin_2"

sombra_hood_ns = cg.esphome_ns.namespace("sombra_hood")
SombraHood = sombra_hood_ns.class_("SombraHood", cg.Component)
SombraHoodListener = sombra_hood_ns.class_("SombraHoodListener")

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SombraHood),
            cv.Required(CONF_SCAN_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_LED_PIN_1): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_LED_PIN_2): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_KEY_PIN_1): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_KEY_PIN_2): pins.internal_gpio_output_pin_schema,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    # The firmware relies on the ESP8266 hardware timer1.
    cv.only_on_esp8266,
)

SOMBRA_HOOD_CHILD_SCHEMA = cv.Schema(
    {cv.GenerateID(CONF_SOMBRA_HOOD_ID): cv.use_id(SombraHood)}
)


async def register_sombra_hood_child(var, config):
    hub = await cg.get_variable(config[CONF_SOMBRA_HOOD_ID])
    cg.add(var.set_parent(hub))
    cg.add(hub.register_listener(var))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_scan_pin(await cg.gpio_pin_expression(config[CONF_SCAN_PIN])))
    cg.add(
        var.set_led_pins(
            await cg.gpio_pin_expression(config[CONF_LED_PIN_1]),
            await cg.gpio_pin_expression(config[CONF_LED_PIN_2]),
        )
    )
    cg.add(
        var.set_key_pins(
            await cg.gpio_pin_expression(config[CONF_KEY_PIN_1]),
            await cg.gpio_pin_expression(config[CONF_KEY_PIN_2]),
        )
    )

    if "ota" in CORE.config:
        ota.request_ota_state_listeners()
