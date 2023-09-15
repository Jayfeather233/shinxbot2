#include "friendadd.h"
#include "utils.h"

void friendadd::process(bot *p, Json::Value J)
{
    Json::Value res;
    res["flag"] = J["flag"];
    res["approve"] = true;
    p->cq_send("set_friend_add_request", res);
    p->setlog(LOG::INFO, "Friend add: " + std::to_string(J["user_id"].asInt64()));
}
bool friendadd::check(bot *p, Json::Value J)
{
    return J.isMember("request_type") &&
           J["request_type"].asString() == "friend";
}
