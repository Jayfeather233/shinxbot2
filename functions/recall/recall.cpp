#include "recall.h"
#include "utils.h"

void recall::process(shinx_message msg)
{
    size_t pos = msg.message.find("[CQ:reply,id=");
    pos += 13;
    int64_t fn = 1, cnt = 0;
    while (msg.message[pos] < '0' || '9' < msg.message[pos]) {
        if (msg.message[pos] == '-')
            fn = -1;
        pos++;
    }
    while ('0' <= msg.message[pos] && msg.message[pos] <= '9') {
        cnt = (cnt << 3) + (cnt << 1) + msg.message[pos] - '0';
        pos++;
    }
    Json::Value J;
    J["message_id"] = fn * cnt;
    cq_send("delete_msg", J);
}
bool recall::check(shinx_message msg)
{
    return (msg.message.find("[CQ:reply,id=") != msg.message.npos &&
            msg.message.find("recall") != msg.message.npos);
}
std::string recall::help() { return "撤回消息：回复某句话输入recall"; }
