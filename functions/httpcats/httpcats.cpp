#include "httpcats.h"
#include "utils.h"

#include <iostream>

void httpcats::process(shinx_message msg)
{
    int64_t code = get_userid(msg.message);
    setlog(LOG::INFO, "httpcats at group " + std::to_string(msg.group_id) +
                          " by " + std::to_string(msg.user_id));
    msg.message = "[CQ:image,file=https://httpcats.com/" +
                  std::to_string(code) + ".jpg,id=40000]";
    cq_send(msg);
}
bool httpcats::check(shinx_message msg)
{
    return msg.message.find("httpcat") == 0;
}
std::string httpcats::help()
{
    return "Http status code with cats. httpcats 404";
}
