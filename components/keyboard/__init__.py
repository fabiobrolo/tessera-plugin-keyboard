"""ESPHome component registration and code generation for the Tessera Keyboard plugin."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import smart_display
from esphome.const import CONF_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["smart_display"]
CODEOWNERS = ["@fabiobrolo"]

keyboard_ns = cg.esphome_ns.namespace("keyboard")
Keyboard = keyboard_ns.class_("Keyboard", cg.Component)
OpenAction = keyboard_ns.class_("OpenAction", automation.Action)

CONF_SCRIPT = "script"
CONF_ON_SUBMIT = "on_submit"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Keyboard),
    cv.Optional(CONF_SCRIPT, default=""): cv.string,
    cv.Optional(CONF_ON_SUBMIT): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(automation.Trigger.template(cg.std_string)),
    }),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Instantiate the Keyboard C++ component, register it with smart_display, and bind automations."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await smart_display.register_plugin(var, __file__)
    cg.add(var.set_script(config[CONF_SCRIPT]))

    for conf in config.get(CONF_ON_SUBMIT, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_submit_trigger(trigger))
        await automation.build_automation(trigger, [(cg.std_string, "query")], conf)


@automation.register_action(
    "keyboard.open",
    OpenAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(Keyboard),
    }),
    synchronous=True,
)
async def keyboard_open_to_code(config, action_id, template_arg, args):
    """Register the keyboard.open ESPHome action."""
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
