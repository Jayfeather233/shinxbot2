#pragma once

#include <cstdint>
#include <jsoncpp/json/json.h>
#include <string>

namespace internal_message {
enum Type : int64_t {
    kUnknown = 0,
    kMemberChangeWelcome = 1000,
    kApproveFriendRequest = 1001,
    kApproveGroupInvite = 1002,
};

inline Json::Value make(Type type, const std::string &payload,
                        uint64_t group_id, uint64_t user_id)
{
    Json::Value w;
    w["post_type"] = "message";
    w["message_type"] = "internal";
    w["message"] = payload;
    w["message_id"] = static_cast<int64_t>(type);
    w["group_id"] = group_id;
    w["user_id"] = user_id;
    return w;
}
} // namespace internal_message
