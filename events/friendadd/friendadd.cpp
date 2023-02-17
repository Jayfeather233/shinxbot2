#include "friendadd.h"
#include "utils.h"

void friendadd::process(Json::Value J){
    Json::Value res;
    res["flag"] = J["flag"];
    res["approve"] = true;
    cq_send("set_friend_add_request", res);
    setlog(LOG::INFO, "Friend add: " + std::to_string(J["user_id"].asInt64()));
}
bool friendadd::check(Json::Value J){
    return J.isMember("request_type") && J["request_type"].asString() == "friend";
}