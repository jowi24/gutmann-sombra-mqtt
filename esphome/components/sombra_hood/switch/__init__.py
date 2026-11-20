import esphome.codegen as cg
from esphome.components import switch

from .. import (
    SOMBRA_HOOD_CHILD_SCHEMA,
    SombraHoodListener,
    register_sombra_hood_child,
    sombra_hood_ns,
)

DEPENDENCIES = ["sombra_hood"]

SombraHoodTimerSwitch = sombra_hood_ns.class_(
    "SombraHoodTimerSwitch", switch.Switch, SombraHoodListener
)

CONFIG_SCHEMA = switch.switch_schema(
    SombraHoodTimerSwitch, icon="mdi:timer", block_inverted=True
).extend(SOMBRA_HOOD_CHILD_SCHEMA)


async def to_code(config):
    var = await switch.new_switch(config)
    await register_sombra_hood_child(var, config)
