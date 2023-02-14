#include "utils.h"

#include <iostream>
#include <jsoncpp/json/json.h>
#include <map>

std::map<long, std::string> username;

void username_init(){
    Json::Value friend_list = string_to_json(do_post("http://127.0.0.1:5750/get_friend_list", "{}"));
    friend_list = friend_list["data"];
    Json::ArrayIndex sz = friend_list.size();
    for(Json::ArrayIndex i = 0; i < sz; ++i){
        username[friend_list[i]["user_id"].asInt64()] = friend_list[i]["nickname"].asString();
    }
}

std::string get_username(long user_id, long group_id){
    if(group_id == -1){
        std::map<long, std::string>::iterator it = username.find(user_id);
        if(it != username.end()){
            return it->second;
        } else {
            Json::Value input;
            input["user_id"] = user_id;
            std::string res = do_post("http://127.0.0.1:5750/get_stranger_info", input);
            input.clear();
            input = string_to_json(res);
            username[user_id] = input["data"]["nickname"].asString();
            return input["data"]["nickname"].asString();
        }
    } else {
        Json::Value input;
        input["user_id"] = user_id;
        input["group_id"] = group_id;
        std::string res = do_post("http://127.0.0.1:5750/get_group_member_info", input);
        input.clear();
        input = string_to_json(res);
        return input["data"].isMember("card")
                ? input["data"]["card"].asString()
                : input["data"]["nickname"].asString();
    }
}