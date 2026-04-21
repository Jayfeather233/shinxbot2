#include "friendadd.h"
#include "internal_message.hpp"

void friendadd::process(bot *p, Json::Value J)
{
    if (!J.isMember("flag")) {
        return;
    }

    Json::Value w = internal_message::make(
        internal_message::kApproveFriendRequest, J["flag"].asString(), 0,
        J.isMember("user_id") ? J["user_id"].asUInt64() : 0);

    p->input_process(w.toStyledString());
}
bool friendadd::check(bot *p, Json::Value J)
{
    (void)p;
    return J.isMember("post_type") && J.isMember("request_type") &&
           J.isMember("sub_type") && J.isMember("flag") &&
           J.isMember("user_id") && J["post_type"].asString() == "request" &&
           J["request_type"].asString() == "friend" &&
           J["sub_type"].asString() == "add";
}

DECLARE_FACTORY_FUNCTIONS(friendadd)