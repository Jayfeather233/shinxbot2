#include "event_plugin_template.h"

#include "utils.h"

bool event_plugin_template::check(bot *p, Json::Value J)
{
    (void)p;
    return J.isMember("post_type") && J.isMember("notice_type") &&
           J["post_type"].asString() == "notice" &&
           J["notice_type"].asString() == "notify";
}

void event_plugin_template::process(bot *p, Json::Value J)
{
    (void)J;
    p->setlog(LOG::INFO, "event_plugin_template handled one event");
}

DECLARE_FACTORY_FUNCTIONS(event_plugin_template)
