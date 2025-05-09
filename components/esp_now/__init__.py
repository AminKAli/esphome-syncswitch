import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.const import (
    CONF_ID,
    CONF_MAC_ADDRESS,
    CONF_TRIGGER_ID,  
    CONF_DATA
)
from esphome.core import CORE

DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["network"]

esp_now_ns = cg.esphome_ns.namespace("esp_now")
ESPNowComponent = esp_now_ns.class_("ESPNowComponent", cg.Component)
ESPNowPeer = esp_now_ns.class_("ESPNowPeer")
ESPNowMessageReceivedTrigger = esp_now_ns.class_("ESPNowMessageReceivedTrigger", automation.Trigger.template(cg.std_string, cg.std_string))
ESPNowSendAction = esp_now_ns.class_("ESPNowSendAction", automation.Action)
ESPNowBroadcastAction = esp_now_ns.class_("ESPNowBroadcastAction", automation.Action)

CONF_ESP_NOW_ID = "esp_now_id"
CONF_PEERS = "peers"
CONF_CHANNEL = "channel"
CONF_ENCRYPTION_KEY = "encryption_key"
CONF_ON_MESSAGE_RECEIVED = "on_message_received"
CONF_BROADCAST = "broadcast"

def validate_mac_address(value):
    """Validate the format of the MAC address."""
    if isinstance(value, str) and len(value.split(':')) == 6:
        parts = value.split(':')
        for part in parts:
            try:
                int(part, 16)
            except ValueError:
                raise cv.Invalid(f"Invalid MAC address part: {part}")
        return value
    raise cv.Invalid("MAC Address must be in format XX:XX:XX:XX:XX:XX")

def validate_channel(value):
    """Validate the WiFi channel (1-14)."""
    return cv.int_range(min=1, max=14)(value)

PEER_SCHEMA = cv.Schema({
    cv.Required(CONF_MAC_ADDRESS): validate_mac_address,
    cv.Optional(CONF_CHANNEL, default=1): validate_channel,
    cv.Optional(CONF_ENCRYPTION_KEY): cv.string
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ESPNowComponent),
    cv.Optional(CONF_PEERS): cv.ensure_list(PEER_SCHEMA),
    cv.Optional(CONF_ON_MESSAGE_RECEIVED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ESPNowMessageReceivedTrigger),
    }),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    if CONF_PEERS in config:
        for peer_config in config[CONF_PEERS]:
            peer = cg.new_variable(peer_config[CONF_MAC_ADDRESS].replace(':', '_'), ESPNowPeer())
            cg.add(peer.set_mac_address(peer_config[CONF_MAC_ADDRESS]))
            cg.add(peer.set_channel(peer_config[CONF_CHANNEL]))
            
            if CONF_ENCRYPTION_KEY in peer_config:
                cg.add(peer.set_encryption_key(peer_config[CONF_ENCRYPTION_KEY]))
                
            cg.add(var.add_peer(peer))
    
    for conf in config.get(CONF_ON_MESSAGE_RECEIVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "mac"), (cg.std_string, "data")], conf)

@automation.register_action(
    "esp_now.send",
    ESPNowSendAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(ESPNowComponent),
        cv.Required(CONF_MAC_ADDRESS): validate_mac_address,
        cv.Required(CONF_DATA): cv.templatable(cv.string),
    })
)
async def esp_now_send_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    
    template_ = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    cg.add(var.set_data(template_))
    cg.add(var.set_mac_address(config[CONF_MAC_ADDRESS]))
    
    return var

@automation.register_action(
    "esp_now.broadcast",
    ESPNowBroadcastAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(ESPNowComponent),
        cv.Required(CONF_DATA): cv.templatable(cv.string),
    })
)
async def esp_now_broadcast_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    
    template_ = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    cg.add(var.set_data(template_))
    
    return var