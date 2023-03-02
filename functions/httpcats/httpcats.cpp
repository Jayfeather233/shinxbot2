#include "httpcats.h"
#include "utils.h"

#include <iostream>

void httpcats::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    int64_t code = get_userid(message);
    setlog(LOG::INFO, "httpcats at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
    cq_send("[CQ:image,file=https://httpcats.com/" + std::to_string(code) + ".jpg,id=40000]", message_type, user_id, group_id);
}
bool httpcats::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find("httpcat")==0;
}
std::string httpcats::help(){
    return "Http status code with cats. httpcats 404";
}

