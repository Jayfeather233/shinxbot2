#include "utils.h"

#include <iostream>
#include <jsoncpp/json/json.h>
#include <map>

std::string get_stranger_info(int64_t user_id){
    Json::Value input;
    input["user_id"] = user_id;
    std::string res = cq_send("get_stranger_info", input);
    input.clear();
    input = string_to_json(res);
    return input["data"]["nickname"].asString();
}

std::string get_group_member_info(int64_t user_id, int64_t group_id){
    Json::Value input;
    input["user_id"] = user_id;
    input["group_id"] = group_id;
    std::string res = cq_send("get_group_member_info", input);
    input.clear();
    input = string_to_json(res);
    if(input["data"].isNull()) return get_stranger_info(user_id);
    else
        return input["data"]["card"].asString().size() != 0
                ? input["data"]["card"].asString()
                : input["data"]["nickname"].asString();
}

std::string get_username(int64_t user_id, int64_t group_id){
    if(group_id == -1){
        return get_stranger_info(user_id);
    } else {
        return get_group_member_info(user_id, group_id);
    }
}