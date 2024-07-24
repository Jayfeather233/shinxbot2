#include "forwarder.h"
#include "utils.h"

#include <iostream>

inline bool check_valid(const point_t &a, const point_t &b)
{
    return (a.first == 0 || b.first == 0 || a.first == b.first) &&
           (a.second == 0 || b.second == 0 || a.second == b.second);
}

forwarder::forwarder()
{
    Json::Value J = string_to_json(readfile("./config/forwarder.json", "[]"));
    for (Json::Value j : J) {
        point_t from, to;
        from = std::make_pair<groupid_t, userid_t>(
            j["from"]["group_id"].asUInt64(), j["from"]["user_id"].asUInt64());
        to = std::make_pair<groupid_t, userid_t>(j["to"]["group_id"].asUInt64(),
                                                 j["to"]["user_id"].asUInt64());
        forward_set.insert(std::make_pair(from, to));
    }
}

void forwarder::save()
{
    Json::Value Ja;
    for (auto it : forward_set) {
        Json::Value J;
        J["from"]["group_id"] = it.first.first;
        J["from"]["user_id"] = it.first.second;
        J["to"]["group_id"] = it.second.first;
        J["to"]["user_id"] = it.second.second;
        Ja.append(J);
    }
    writefile("./config/forwarder.json", Ja.toStyledString());
}

size_t forwarder::configure(std::string message, const msg_meta &conf)
{
    size_t t;
    if (message.find("forward.set") == 0) {
        std::istringstream iss(message.substr(11));
        point_t from, to;
        iss >> from.first >> from.second >> to.first >> to.second;
        if (conf.p->is_op(conf.user_id) ||
            ((from.first == 0 ||
              is_group_op(conf.p, from.first, conf.user_id)) &&
             ((to.first == 0 && to.second == conf.user_id) ||
              (to.first == conf.group_id &&
               is_group_op(conf.p, conf.group_id, conf.user_id))))) {
            t = forward_set.insert(std::make_pair(from, to)).second;
        }
        else {
            t = 0;
        }
    }
    else if (message.find("forward.del") == 0) {
        std::istringstream iss(message.substr(11));
        point_t from, to;
        iss >> from.first >> from.second >> to.first >> to.second;
        if (conf.p->is_op(conf.user_id) ||
            ((from.first == 0 ||
              is_group_op(conf.p, from.first, conf.user_id)) &&
             ((to.first == 0 && to.second == conf.user_id) ||
              (to.first == conf.group_id &&
               is_group_op(conf.p, conf.group_id, conf.user_id))))) {
            t = forward_set.erase(std::make_pair(from, to));
        }
        else {
            t = 0;
        }
    }
    else
        t = -1;
    save();
    return t;
}

bool is_full_msg(const std::string &message)
{
    return message.find("[CQ:vedio") == 0 || message.find("[CQ:record") == 0 ||
           message.find("[CQ:share") == 0 || message.find("[CQ:contact") == 0 ||
           message.find("[CQ:location") == 0 ||
           message.find("[CQ:forward") == 0 || message.find("[CQ:xml") == 0 ||
           message.find("[CQ:json") == 0;
}

void forwarder::process(std::string message, const msg_meta &conf)
{
    if (trim(message) == "forward.help") {
        conf.p->cq_send(
            "forwarder.[set/del] src_group_id src_user_id dst_group_id "
            "dst_user_id\n"
            "If src_group_id == 0, that means all the message by that user\n"
            "If src_user_id == 0, that means all the message in that group\n"
            "If dst_group_id == 0, that means forward to private chat\n"
            "Else then forward to group chat.\n"
            "You can only forward msg from the group you manage to yourself or "
            "another group you manage.",
            conf);
        return;
    }
    if (message.find("forward.") == 0) {
        int t = configure(message, conf);
        conf.p->cq_send("done with code: " + std::to_string(t), conf);
        return;
    }
    point_t fr = std::make_pair(conf.group_id, conf.user_id);
    std::string group_name, user_name, all_msg;
    bool flg = false;
    for (auto it : forward_set) {
        if (check_valid(it.first, fr)) {
            if (!flg && it.first.first != 0) {
                Json::Value qst;
                qst["group_id"] = it.first.first;
                qst = string_to_json(conf.p->cq_send("get_group_info", qst));
                if (qst["data"].isNull())
                    group_name = "";
                else
                    group_name = qst["data"]["group_name"].asString();
                flg = true;
            }
            user_name = get_username(conf.p, conf.user_id, conf.group_id) +
                        "(" + std::to_string(conf.user_id) + "): ";
            all_msg = it.first.first == 0 ? user_name
                                          : (group_name + " " + user_name);

            if (is_full_msg(message)) {
                Json::Value Ja;
                Json::Value J2;
                J2["type"] = "node";
                J2["data"]["id"] = conf.message_id;
                Ja.append(J2);
                if (it.second.first == 0) {
                    conf.p->cq_send(
                        all_msg, (msg_meta){"private", it.second.second, 0, 0});
                    Json::Value J;
                    J["user_id"] = it.second.second;
                    J["messages"] = Ja;
                    conf.p->cq_send("send_private_forward_msg", J);
                }
                else {
                    conf.p->cq_send(all_msg,
                                    (msg_meta){"group", 0, it.second.first, 0});
                    Json::Value J;
                    J["group_id"] = it.second.first;
                    J["messages"] = Ja;
                    conf.p->cq_send("send_group_forward_msg", J);
                }
            }
            else {
                if (it.second.first == 0) {
                    conf.p->cq_send(
                        all_msg + message,
                        (msg_meta){"private", it.second.second, 0, 0});
                }
                else {
                    conf.p->cq_send(all_msg + message,
                                    (msg_meta){"group", 0, it.second.first, 0});
                }
            }
        }
    }
}
bool forwarder::check(std::string message, const msg_meta &conf)
{
    return true;
}
std::string forwarder::help() { return ""; }

extern "C" processable *create() { return new forwarder(); }