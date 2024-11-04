#include "recall.h"
#include "utils.h"

void recall::process(std::string message, const msg_meta &conf)
{
    size_t pos = message.find("[CQ:reply,id=");
    pos += 13;
    int64_t fn = 1, cnt = 0;
    while (message[pos] < '0' || '9' < message[pos]) {
        if (message[pos] == '-')
            fn = -1;
        pos++;
    }
    while ('0' <= message[pos] && message[pos] <= '9') {
        cnt = (cnt << 3) + (cnt << 1) + message[pos] - '0';
        pos++;
    }
    Json::Value J;
    J["message_id"] = fn * cnt;
    conf.p->cq_send("delete_msg", J);
    conf.p->setlog(LOG::INFO, fmt::format("recall msg by {}", conf.user_id));
}
bool recall::check(std::string message, const msg_meta &conf)
{
    return (message.find("[CQ:reply,id=") != message.npos &&
            message.find("recall") != message.npos &&
            (conf.p->is_op(conf.user_id) ||
             is_group_op(conf.p, conf.group_id, conf.user_id)));
}
std::string recall::help() { return "撤回消息：回复某句话输入recall"; }

DECLARE_FACTORY_FUNCTIONS(recall)