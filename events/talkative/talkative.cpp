#include "talkative.h"
#include "utils.h"

void talkative::process(Json::Value J){
    setlog(LOG::INFO, "talkative at group " + std::to_string(J["group_id"].asInt64()));
    cq_send((shinx_message){"[CQ:at,qq=" + std::to_string(J["user_id"].asInt64()) + "] 获得了龙王标识！", "group", 0, J["group_id"].asInt64(), 0});
}
bool talkative::check(Json::Value J){
    return J.isMember("honor_type") && J["honor_type"].asString() == "talkative";
}