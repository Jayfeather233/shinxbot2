#include "httpcats.h"
#include "utils.h"

#include <iostream>

void httpcats::process(std::string message, const msg_meta &conf)
{
    Json::Value J;
    J["message_id"] = conf.message_id;
    conf.p->cq_send("mark_msg_as_read", J);
    int64_t code = std::stoi(message.substr(8, message.size() - 8));
    code = (code < 100 || code > 599) ? 404 : code; // invalid code
    conf.p->setlog(LOG::INFO, "httpcats at group " +
                                  std::to_string(conf.group_id) + " by " +
                                  std::to_string(conf.user_id));
    conf.p->cq_send("[CQ:image,file=https://httpcats.com/" +
                        std::to_string(code) + ".jpg,id=40000]",
                    conf);
}
bool httpcats::check(std::string message, const msg_meta &conf)
{
    return message.find("httpcat") == 0;
}
std::string httpcats::help()
{
    return "Http status code with cats. httpcats 404";
}

DECLARE_FACTORY_FUNCTIONS(httpcats)
