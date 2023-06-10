#include "forwarder.h"
#include "utils.h"

#include <iostream>

inline bool check_valid(const point_t &a, const point_t &b)
{
    return (a.first == -1 || b.first == -1 || a.first == b.first) &&
           (a.second == -1 || b.second == -1 || a.second == b.second);
}

forwarder::forwarder()
{
    Json::Value J = string_to_json(readfile("./config/forwarder.json", "[]"));
    for (Json::Value j : J) {
        point_t from, to;
        from = std::make_pair<int64_t, int64_t>(j["from"]["group_id"].asInt64(),
                                                j["from"]["user_id"].asInt64());
        to = std::make_pair<int64_t, int64_t>(j["to"]["group_id"].asInt64(),
                                              j["to"]["user_id"].asInt64());
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
        if (is_op(conf.user_id) ||
            ((from.first == -1 || is_group_op(from.first, conf.user_id)) &&
             ((to.first == -1 && to.second == conf.user_id) ||
              (to.first == conf.group_id &&
               is_group_op(conf.group_id, conf.user_id))))) {
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
        if (is_op(conf.user_id) ||
            ((from.first == -1 || is_group_op(from.first, conf.user_id)) &&
             ((to.first == -1 && to.second == conf.user_id) ||
              (to.first == conf.group_id &&
               is_group_op(conf.group_id, conf.user_id))))) {
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
        cq_send(
            "forwarder.[set/del] src_group_id src_user_id dst_group_id "
            "dst_user_id\n"
            "If src_group_id == -1, that means all the message by that user\n"
            "If src_user_id == -1, that means all the message in that group\n"
            "If dst_group_id == -1, that means forward to private chat\n"
            "Else then forward to group chat.\n"
            "You can only forward msg from the group you manage to yourself or "
            "another group you manage.",
            conf);
        return;
    }
    if (message.find("forward.") == 0) {
        int t = configure(message, conf);
        cq_send("done with code: " + std::to_string(t), conf);
        return;
    }
    point_t fr = std::make_pair(conf.group_id, conf.user_id);
    std::string group_name, user_name, all_msg;
    bool flg = false;
    for (auto it : forward_set) {
        if (check_valid(it.first, fr)) {
            if (!flg && it.first.first != -1) {
                Json::Value qst;
                qst["group_id"] = it.first.first;
                group_name = string_to_json(cq_send("get_group_info",
                                                    qst))["data"]["group_name"]
                                 .asString();
                flg = true;
            }
            user_name = get_username(conf.user_id, conf.group_id) + "(" +
                        std::to_string(conf.user_id) + "): ";
            all_msg = it.first.first == -1 ? user_name
                                           : (group_name + " " + user_name);

            if (is_full_msg(message)) {
                Json::Value Ja;
                Json::Value J2;
                J2["type"] = "node";
                J2["data"]["id"] = conf.message_id;
                Ja.append(J2);
                if (it.second.first == -1) {
                    cq_send(all_msg,
                            (msg_meta){"private", it.second.second, -1, 0});
                    Json::Value J;
                    J["user_id"] = it.second.second;
                    J["messages"] = Ja;
                    cq_send("send_private_forward_msg", J);
                }
                else {
                    cq_send(all_msg,
                            (msg_meta){"group", -1, it.second.first, 0});
                    Json::Value J;
                    J["group_id"] = it.second.first;
                    J["messages"] = Ja;
                    cq_send("send_group_forward_msg", J);
                }
            } else {
                if (it.second.first == -1) {
                    cq_send(all_msg + message,
                            (msg_meta){"private", it.second.second, -1, 0});
                }
                else {
                    cq_send(all_msg + message,
                            (msg_meta){"group", -1, it.second.first, 0});
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