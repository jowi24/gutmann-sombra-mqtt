import esphome.codegen as cg
from esphome.components import button
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import CONF_SOMBRA_HOOD_ID, SOMBRA_HOOD_CHILD_SCHEMA, sombra_hood_ns

DEPENDENCIES = ["sombra_hood"]

SombraHoodFilterResetButton = sombra_hood_ns.class_(
    "SombraHoodFilterResetButton", button.Button
)

CONFIG_SCHEMA = button.button_schema(
    SombraHoodFilterResetButton,
    icon="mdi:air-filter",
    entity_category=ENTITY_CATEGORY_CONFIG,
).extend(SOMBRA_HOOD_CHILD_SCHEMA)


async def to_code(config):
    var = await button.new_button(config)
    hub = await cg.get_variable(config[CONF_SOMBRA_HOOD_ID])
    cg.add(var.set_parent(hub))
