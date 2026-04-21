#include "groupinvite.h"
#include "internal_message.hpp"

void groupinvite::process(bot *p, Json::Value J)
{
    if (!J.isMember("flag")) {
        return;
    }

    Json::Value w = internal_message::make(
        internal_message::kApproveGroupInvite, J["flag"].asString(),
        J.isMember("group_id") ? J["group_id"].asUInt64() : 0,
        J.isMember("user_id") ? J["user_id"].asUInt64() : 0);

    p->input_process(w.toStyledString());
}

bool groupinvite::check(bot *p, Json::Value J)
{
    (void)p;
    return J.isMember("post_type") && J.isMember("request_type") &&
           J.isMember("sub_type") && J.isMember("flag") &&
           J.isMember("group_id") && J.isMember("user_id") &&
           J["post_type"].asString() == "request" &&
           J["request_type"].asString() == "group" &&
           J["sub_type"].asString() == "invite";
}

DECLARE_FACTORY_FUNCTIONS(groupinvite)
