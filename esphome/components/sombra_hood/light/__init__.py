import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_OUTPUT_ID

from .. import (
    SOMBRA_HOOD_CHILD_SCHEMA,
    SombraHoodListener,
    register_sombra_hood_child,
    sombra_hood_ns,
)

DEPENDENCIES = ["sombra_hood"]

SombraHoodLight = sombra_hood_ns.class_(
    "SombraHoodLight", light.LightOutput, SombraHoodListener
)

CONFIG_SCHEMA = light.BINARY_LIGHT_SCHEMA.extend(
    {cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SombraHoodLight)}
).extend(SOMBRA_HOOD_CHILD_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await register_sombra_hood_child(var, config)
