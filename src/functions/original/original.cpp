#include "original.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <map>
static std::map<userid_t, bool> in_queue;

void original::process(std::string message, const msg_meta &conf)
{
    const std::string normalized = trim(message);
    if (normalized == ".original.help") {
        conf.p->cq_send("消息解析帮助\n.original后发送要解析的消息，或用."
                        "original回复需要解析的消息",
                        conf);
        return;
    }

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
        // for (const char &ch : message) {
        //     if (ch == '[')
        //         response += "&#91;";
        //     else if (ch == ']')
        //         response += "&#93;";
        //     else
        //         response += ch;
        // }
        response = cq_encode(message);
        conf.p->cq_send(response, conf);
        conf.p->setlog(LOG::INFO, "original at group " +
                                      std::to_string(conf.group_id) + " by " +
                                      std::to_string(conf.user_id));
    }
}
bool original::check(std::string message, const msg_meta &conf)
{
    return cmd_match_prefix(message, {".original"}) ||
           (in_queue.find(conf.user_id) != in_queue.end() &&
            in_queue[conf.user_id] == true);
}
std::string original::help()
{
    return "消息解析：返回qq消息的原始内容。帮助：.original.help";
}

DECLARE_FACTORY_FUNCTIONS(original)