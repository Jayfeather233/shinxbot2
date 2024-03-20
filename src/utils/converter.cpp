#include "utils.h"

bool is_op(const bot *p, const uint64_t a) { return p->is_op(a); }

std::string cq_send(const bot *p, const std::string &message,
                    const msg_meta &conf)
{
    return p->cq_send(message, conf);
}

std::string cq_send(const bot *p, const std::string &end_point,
                    const Json::Value &J)
{
    return p->cq_send(end_point, J);
}

std::string cq_get(const bot *p, const std::string &end_point)
{
    return p->cq_get(end_point);
}

void setlog(bot *p, LOG type, std::string message) { p->setlog(type, message); }

uint64_t get_botqq(const bot *p) { return p->get_botqq(); }

std::string get_local_path() { return std::filesystem::current_path(); }

void input_process(bot *p, std::string *input) { p->input_process(input); }

std::string message_to_string(const Json::Value &J)
{
    if (J["type"].asString() == "text") {
        return J["data"]["text"].asString();
    }
    else {
        std::string ret = "[CQ:" + J["type"].asString();
        for (auto u : J["data"].getMemberNames()) {
            ret += "," + u + "=" + cq_encode(J["data"][u].asString());
        }
        ret += "]";
        return ret;
    }
}

std::string messageArr_to_string(const Json::Value &J)
{
    if (J.isString()) {
        return J.asString();
    }
    else if (J.isArray()) {
        std::string ret;
        Json::ArrayIndex sz = J.size();
        for (Json::ArrayIndex i = 0; i < sz; i++) {
            ret += message_to_string(J[i]);
        }
        return ret;
    }
    else {
        return message_to_string(J);
    }
}