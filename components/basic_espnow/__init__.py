import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MAC_ADDRESS
from esphome import automation

DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["logger"]
CODEOWNERS = ["@yourgithub"]

basic_espnow_ns = cg.esphome_ns.namespace("basic_espnow")
BasicESPNow = basic_espnow_ns.class_("BasicESPNow", cg.Component)

CONF_PEER_MAC = "peer_mac"
CONF_ON_MESSAGE = "on_message"

OnMessageTrigger = basic_espnow_ns.class_("OnMessageTrigger", automation.Trigger.template(cg.std_string))

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BasicESPNow),
    cv.Optional(CONF_PEER_MAC): cv.mac_address,
    cv.Optional(CONF_ON_MESSAGE): automation.validate_automation({cv.GenerateID(): cv.use_id(BasicESPNow)}),
}).extend(cv.COMPONENT_SCHEMA)

@cg.register_component
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if CONF_PEER_MAC in config:
        mac = config[CONF_PEER_MAC]
        cg.add(var.set_peer_mac(mac))

    # Register the 'send_espnow' service
    cg.add(var.register_service("send_espnow", {"message": cg.std_string}))

    if CONF_ON_MESSAGE in config:
        for conf in config[CONF_ON_MESSAGE]:
            trigger = cg.new_Pvariable(conf[automation.CONF_TRIGGER_ID], OnMessageTrigger(var))
            await automation.build_automation(trigger, [(cg.std_string, "message")], conf)

    return var