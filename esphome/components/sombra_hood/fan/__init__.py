import esphome.codegen as cg
from esphome.components import fan

from .. import (
    SOMBRA_HOOD_CHILD_SCHEMA,
    SombraHoodListener,
    register_sombra_hood_child,
    sombra_hood_ns,
)

DEPENDENCIES = ["sombra_hood"]

SombraHoodFan = sombra_hood_ns.class_(
    "SombraHoodFan", cg.Component, fan.Fan, SombraHoodListener
)

CONFIG_SCHEMA = fan.fan_schema(SombraHoodFan, icon="mdi:fan").extend(
    SOMBRA_HOOD_CHILD_SCHEMA
)


async def to_code(config):
    var = await fan.new_fan(config)
    await cg.register_component(var, {})
    await register_sombra_hood_child(var, config)
