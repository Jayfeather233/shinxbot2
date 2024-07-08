#include "talkative.h"
#include "utils.h"

void talkative::process(bot *p, Json::Value J)
{
    p->setlog(LOG::INFO,
           "talkative at group " + std::to_string(J["group_id"].asUInt64()));
    p->cq_send("[CQ:at,qq=" + std::to_string(J["user_id"].asUInt64()) +
                "] 获得了龙王标识！",
            (msg_meta){"group", 0, J["group_id"].asUInt64(), 0});
}
bool talkative::check(bot *p, Json::Value J)
{
    return J.isMember("honor_type") &&
           J["honor_type"].asString() == "talkative";
}

extern "C" eventprocess* create() {
    return new talkative();
}