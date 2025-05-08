import esphome.codegen as cg

CODEOWNERS = ["@AminKAli"]
basic_espnow_ns = cg.esphome_ns.namespace("basic_espnow")
BasicESPNow = basic_espnow_ns.class_("BasicESPNow", cg.Component)

CONFIG_SCHEMA = cg.Schema({}).extend(cg.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable("basic_espnow", BasicESPNow())
    yield cg.register_component(var, config)
