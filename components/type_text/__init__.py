import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import smart_display
from esphome.const import CONF_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["smart_display"]
CODEOWNERS = ["@fabiobrolo"]

type_text_ns = cg.esphome_ns.namespace("type_text")
TypeText = type_text_ns.class_("TypeText", cg.Component)
OpenAction = type_text_ns.class_("OpenAction", automation.Action)

CONF_SCRIPT = "script"
CONF_ON_SUBMIT = "on_submit"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TypeText),
    cv.Optional(CONF_SCRIPT, default=""): cv.string,
    cv.Optional(CONF_ON_SUBMIT): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(automation.Trigger.template(cg.std_string)),
    }),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await smart_display.register_plugin(var, __file__)
    cg.add(var.set_script(config[CONF_SCRIPT]))

    for conf in config.get(CONF_ON_SUBMIT, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_submit_trigger(trigger))
        await automation.build_automation(trigger, [(cg.std_string, "query")], conf)


@automation.register_action(
    "type_text.open",
    OpenAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(TypeText),
    }),
    synchronous=True,
)
async def type_text_open_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
