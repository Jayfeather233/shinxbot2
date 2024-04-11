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

Json::Value string_to_message(const std::string &s)
{
    Json::Value J;
    if (s[0] == '[') {
        size_t pos = s.find(','), laspos, mid;
        J["type"] = s.substr(4, pos - 4);
        laspos = pos;
        while (pos != s.length() - 1) {
            pos = s.find(',', pos + 1);
            if (pos == s.npos) {
                pos = s.length() - 1;
            }
            mid = s.find('=', laspos + 1);
            if (mid == s.npos) {
                throw "CQ code syntax error.";
            }
            J["data"][s.substr(laspos + 1, mid - laspos - 1)] =
                cq_decode(s.substr(mid + 1, pos - mid - 1));
            laspos = pos;
        }
    }
    else {
        J["type"] = "text";
        J["data"]["text"] = s;
    }
    return J;
}

Json::Value string_to_messageArr(const std::string &s)
{
    size_t pos = 0, laspos = 0;
    Json::Value J;
    while ((pos = s.find('[', pos)) != s.npos) {
        if (laspos != pos) {
            J.append(string_to_message(s.substr(laspos, pos - laspos)));
            laspos = pos;
        }
        pos = s.find(']', pos);
        if (pos == s.npos) {
            throw "missing match for \'[\' in message: " + s;
        }
        pos += 1;
        J.append(string_to_message(s.substr(laspos, pos - laspos)));
        laspos = pos;
    }
    J.append(string_to_message(s.substr(laspos)));
    return J;
}