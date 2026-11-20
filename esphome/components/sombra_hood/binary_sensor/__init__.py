import esphome.codegen as cg
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PROBLEM

from .. import (
    SOMBRA_HOOD_CHILD_SCHEMA,
    SombraHoodListener,
    register_sombra_hood_child,
    sombra_hood_ns,
)

DEPENDENCIES = ["sombra_hood"]

SombraHoodMaintenanceSensor = sombra_hood_ns.class_(
    "SombraHoodMaintenanceSensor", binary_sensor.BinarySensor, SombraHoodListener
)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(
    SombraHoodMaintenanceSensor,
    device_class=DEVICE_CLASS_PROBLEM,
    icon="mdi:air-filter",
).extend(SOMBRA_HOOD_CHILD_SCHEMA)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await register_sombra_hood_child(var, config)
