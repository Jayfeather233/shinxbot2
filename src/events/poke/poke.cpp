#include "poke.h"
#include "utils.h"

void poke::process(bot *p, Json::Value J)
{
    p->cq_send("[CQ:poke,qq=" + std::to_string(J["user_id"].asUInt64()) + "]",
            (msg_meta){"group", 0, J["group_id"].asUInt64(), 0});
    if (get_random(3) == 0)
        p->cq_send("别戳我TAT",
                (msg_meta){"group", 0, J["group_id"].asUInt64(), 0});
}
bool poke::check(bot *p, Json::Value J)
{
    if (!J.isMember("group_id") || !J.isMember("sub_type") ||
        J["sub_type"].asString() != "poke")
        return false;
    uint64_t target_id = J["target_id"].asUInt64();
    uint64_t user_id = J["user_id"].asUInt64();
    if (target_id != p->get_botqq() || user_id == 1783241911 ||
        user_id == 1318920100)
        return false;
    return true;
}

extern "C" eventprocess* create() {
    return new poke();
}