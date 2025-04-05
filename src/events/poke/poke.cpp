#include "poke.h"
#include "utils.h"

poke::poke()
{
    Json::Value J = string_to_json(readfile("./config/poke.json", "{\"users\": [], \"groups\": []}"));
    parse_json_to_set(J["users"], no_poke_users);
    parse_json_to_set(J["groups"], no_poke_groups);
    minInterval = std::chrono::seconds(3);
}

void poke::process(bot *p, Json::Value J)
{
    std::unique_lock<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    if (lastExecutionTime_.time_since_epoch().count() != 0) {
        auto elapsedTime = now - lastExecutionTime_;
        if (elapsedTime < minInterval) {
            return;
        }
    }
    lastExecutionTime_ = std::chrono::steady_clock::now();

    // p->cq_send("[CQ:poke,qq=" + std::to_string(J["user_id"].asUInt64()) +
    // "]",
    //         (msg_meta){"group", 0, J["group_id"].asUInt64(), 0});
    Json::Value Jx;
    Jx["group_id"] = J["group_id"];
    Jx["user_id"] = J["user_id"];
    p->cq_send("group_poke", Jx);
    p->cq_send("给你一拳", (msg_meta){"group", 0, J["group_id"].asUInt64(), 0});
}
bool poke::check(bot *p, Json::Value J)
{
    if (!J.isMember("group_id") || !J.isMember("sub_type") ||
        J["sub_type"].asString() != "poke")
        return false;
    userid_t target_id = J["target_id"].asUInt64();
    userid_t user_id = J["user_id"].asUInt64();
    groupid_t group_id = J["group_id"].asUInt64();
    if (target_id != p->get_botqq() ||
        no_poke_users.find(user_id) != no_poke_users.end() || no_poke_groups.find(group_id) != no_poke_groups.end())
        return false;
    return true;
}

DECLARE_FACTORY_FUNCTIONS(poke)