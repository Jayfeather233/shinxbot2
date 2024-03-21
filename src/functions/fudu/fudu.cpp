#include "fudu.h"
#include "utils.h"

#include <map>

void fudu::process(std::string message, const msg_meta &conf)
{
    auto it = gmsg.find(conf.group_id);
    if (it != gmsg.end()) {
        if (message == it->second) {
            times[conf.group_id]++;
            if (times[conf.group_id] == 5) {
                Json::Value J;
                J["message_id"] = conf.message_id;
                conf.p->cq_send("mark_msg_as_read", J);
                conf.p->cq_send(message, conf);
            }
        }
        else {
            gmsg[conf.group_id] = message;
            times[conf.group_id] = 1;
        }
    }
    else {
        gmsg[conf.group_id] = message;
        times[conf.group_id] = 1;
    }
}
bool fudu::check(std::string message, const msg_meta &conf)
{
    return conf.message_type == "group" && conf.user_id != conf.p->get_botqq();
}
std::string fudu::help() { return ""; }
