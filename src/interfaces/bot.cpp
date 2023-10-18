#include "bot.h"
#include "utils.h"

bot::bot(int recv_port, int send_port)
    : receive_port(recv_port), send_port(send_port)
{
}

bool bot::is_op(const int64_t a) const { return false; }

std::string bot::cq_send(const std::string &message, const msg_meta &conf) const
{
    Json::Value input;
    input["message"] = message;
    input["message_type"] = conf.message_type;
    input["group_id"] = conf.group_id;
    input["user_id"] = conf.user_id;
    return this->cq_send("send_msg", input);
}

std::string bot::cq_send(const std::string &end_point,
                         const Json::Value &J) const
{
    return do_post("127.0.0.1:" + std::to_string(send_port) + "/" + end_point,
                   J);
}

std::string bot::cq_get(const std::string &end_point) const
{
    return do_get("127.0.0.1:" + std::to_string(send_port) + "/" + end_point);
}

void bot::setlog(LOG type, std::string message) {}

int64_t bot::get_botqq() const { return botqq; }

bot::~bot() {}