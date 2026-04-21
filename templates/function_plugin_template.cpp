#include "function_plugin_template.h"

#include "utils.h"

namespace {
constexpr const char *CMD_HELP = "*tpl.help";
constexpr const char *CMD_RUN = "*tpl";
} // namespace

bool function_plugin_template::check(std::string message, const msg_meta &conf)
{
    (void)conf;
    const std::string m = trim(message);
    return cmd_match_exact(m, {CMD_HELP, CMD_RUN});
}

void function_plugin_template::process(std::string message,
                                       const msg_meta &conf)
{
    const std::string m = trim(message);

    const std::vector<cmd_exact_rule> exact_rules = {
        {CMD_HELP,
         [&]() {
             conf.p->cq_send(help(conf, help_level_t::public_only), conf);
             return true;
         }},
        {CMD_RUN,
         [&]() {
             conf.p->cq_send("template command handled", conf);
             return true;
         }},
    };

    if (!cmd_dispatch(m, exact_rules, {})) {
        conf.p->cq_send("unknown command, use *tpl.help", conf);
    }
}

std::string function_plugin_template::help()
{
    return "template function: *tpl, help: *tpl.help";
}

std::string function_plugin_template::help(const msg_meta &conf,
                                           help_level_t level)
{
    (void)conf;
    if (level == help_level_t::bot_admin) {
        return "template function (admin): *tpl, *tpl.help";
    }
    return help();
}

bool function_plugin_template::reload(const msg_meta &conf)
{
    (void)conf;
    return true;
}

DECLARE_FACTORY_FUNCTIONS(function_plugin_template)
