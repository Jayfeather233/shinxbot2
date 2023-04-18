#include "poke.h"
#include "utils.h"

void poke::process(Json::Value J){
    cq_send((shinx_message){"[CQ:poke,qq=" + std::to_string(J["user_id"].asInt64()) + "]",
                            "group", 0, J["group_id"].asInt64(), 0});
    if (get_random(3) == 0)
        cq_send((shinx_message){"别戳我TAT", "group", 0, J["group_id"].asInt64(), 0});
}
bool poke::check(Json::Value J){
    if(!J.isMember("group_id") || !J.isMember("sub_type") || J["sub_type"].asString() != "poke")  return false;
    int64_t target_id = J["target_id"].asInt64();
    int64_t user_id = J["user_id"].asInt64();
    if(target_id != get_botqq() || user_id == 1783241911 || user_id == 1318920100) return false;
    return true;
}