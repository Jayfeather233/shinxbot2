#include "httpcats.h"
#include "utils.h"

#include <iostream>

void httpcats::process(std::string message, const msg_meta &conf)
{
    int64_t code = get_userid(message);
    setlog(LOG::INFO, "httpcats at group " + std::to_string(conf.group_id) +
                          " by " + std::to_string(conf.user_id));
    message = "[CQ:image,file=https://httpcats.com/" + std::to_string(code) +
              ".jpg,id=40000]";
    cq_send(message, conf);
}
bool httpcats::check(std::string message, const msg_meta &conf)
{
    return message.find("httpcat") == 0;
}
std::string httpcats::help()
{
    return "Http status code with cats. httpcats 404";
}
