#include "bot.h"
#include "utils.h"

// msg_meta::msg_meta()
//     : message_type(""), user_id(0), group_id(0), message_id(0), p(nullptr)
// {
// }
msg_meta::msg_meta(const msg_meta &u)
    : message_type(u.message_type), user_id(u.user_id), group_id(u.group_id),
      message_id(u.message_id), p(u.p)
{
}
msg_meta::msg_meta(const msg_meta &&u)
    : message_type(u.message_type), user_id(u.user_id), group_id(u.group_id),
      message_id(u.message_id), p(u.p)
{
}
msg_meta::msg_meta(std::string mt, userid_t uid, groupid_t gid, int64_t mid,
                   bot *pp)
    : message_type(mt), user_id(uid), group_id(gid), message_id(mid), p(pp)
{
}

bot::bot(int recv_port, int send_port)
    : receive_port(recv_port), send_port(send_port)
{
}

bool bot::is_op(const userid_t a) const { return false; }

std::string bot::cq_send(const std::string &message, const msg_meta &conf) const
{
    Json::Value input;
    input["message"] = trim(message);
    input["message_type"] = conf.message_type;
    input["group_id"] = conf.group_id;
    input["user_id"] = conf.user_id;
    return this->cq_send("send_msg", input);
}

std::string bot::cq_send(const std::string &end_point,
                         const Json::Value &J) const
{
    return do_post((std::string) "127.0.0.1", send_port,
                   (std::string) "/" + end_point, false, J);
}

std::string bot::cq_get(const std::string &end_point) const
{
    return do_get((std::string) "127.0.0.1", send_port,
                  (std::string) "/" + end_point, false);
}

void bot::setlog(LOG type, std::string message) {}

userid_t bot::get_botqq() const { return botqq; }

bot::~bot() {}