#include "original.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <map>
static std::map<uint64_t, bool> in_queue;

void original::process(std::string message, const msg_meta &conf)
{
    Json::Value J;
    J["message_id"] = conf.message_id;
    conf.p->cq_send("mark_msg_as_read", J);
    if (in_queue.find(conf.user_id) == in_queue.end())
        in_queue[conf.user_id] = false;

    if (!in_queue[conf.user_id]) {
        in_queue[conf.user_id] = true;
        conf.p->cq_send("Please send your message.", conf);
        return;
    }
    else {
        in_queue[conf.user_id] = false;
        std::string response;
        for (const char &ch : message) {
            if (ch == '[')
                response += "&#91;";
            else if (ch == ']')
                response += "&#93;";
            else
                response += ch;
        }
        conf.p->cq_send(response, conf);
        conf.p->setlog(LOG::INFO, "original at group " +
                                      std::to_string(conf.group_id) + " by " +
                                      std::to_string(conf.user_id));
    }
}
bool original::check(std::string message, const msg_meta &conf)
{
    return message.find(".original") == 0 ||
           (in_queue.find(conf.user_id) != in_queue.end() &&
            in_queue[conf.user_id] == true);
}
std::string original::help()
{
    return "return the original text. send .original to begin.";
}

extern "C" processable* create() {
    return new original();
}
