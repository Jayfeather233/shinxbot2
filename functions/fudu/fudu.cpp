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
                times[conf.group_id] = 0;
                cq_send(message, conf);
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
    return conf.message_type == "group" && conf.user_id != get_botqq();
}
std::string fudu::help() { return ""; }
