#include "warn_recorder.h"
#include "utils.h"

#include <iostream>

void warn_recorder::process_command(std::string command, const msg_meta &conf) {
    if (command.find("clear") == 0) {
        userid_t user_id;
        user_id = my_string2uint64(command.substr(5));
        if (user_id == 0) {
            conf.p->cq_send("参数错误。请使用 /warn.clear [qq] 格式", conf);
            return;
        }
        warns[conf.group_id].erase(user_id);
        save();
        conf.p->cq_send("已清除该用户的警告记录", conf);
    }
    else if (command.find("del") == 0) {
        command = trim(command.substr(3));
        size_t space_pos = command.find(' ');
        if (space_pos == std::string::npos) {
            if (command[0] != '[') {
                conf.p->cq_send("参数错误。请使用 /warn.del [qq] [index] 格式", conf);
                return;
            } else {
                space_pos = command.find(']');
            }
        }
        userid_t user_id;
        user_id = my_string2uint64(command.substr(0, space_pos));
        if (user_id == 0) {
            conf.p->cq_send("参数错误。请使用 /warn.del [qq] [index] 格式", conf);
            return;
        }
        size_t index;
        try {
            index = std::stoul(command.substr(space_pos + 1)) - 1;
        } catch (...) {
            conf.p->cq_send("参数错误。请使用 /warn.del [qq] [index] 格式", conf);
            return;
        }
        if (index >= warns[conf.group_id][user_id].size()) {
            conf.p->cq_send("参数错误。索引超出范围", conf);
            return;
        }
        warns[conf.group_id][user_id].erase(warns[conf.group_id][user_id].begin() + index);
        save();
        conf.p->cq_send("已删除该条警告记录", conf);
    }
}

// output 已警告 <user> %s 次\n理由: 1. xxx\n2. xxx
void warn_recorder::process(std::string message, const msg_meta &conf)
{
    message = trim(message.substr(5));
    if (message[0] == '.') {
        process_command(message.substr(1), conf);
        return;
    }
    size_t space_pos = message.find(' ');
    if (space_pos == std::string::npos) {
        if (message[0] != '[') {
            conf.p->cq_send("参数错误。请使用 /warn [qq] [msg] 格式", conf);
            return;
        } else {
            space_pos = message.find(']');
        }
    }
    userid_t user_id;
    user_id = my_string2uint64(message.substr(0, space_pos));
    if (user_id == 0) {
        conf.p->cq_send("参数错误。请使用 /warn [qq] [msg] 格式", conf);
        return;
    }
    std::string warn_msg = trim(message.substr(space_pos + 1));
    warns[conf.group_id][user_id].push_back(warn_msg);
    save();
    std::ostringstream oss;
    oss << fmt::format("已警告 {} {} 次\n", get_username(conf.p, user_id, conf.group_id), warns[conf.group_id][user_id].size());
    for (size_t i = 0; i < warns[conf.group_id][user_id].size(); i++) {
        oss << fmt::format("{}. {}\n", i + 1, warns[conf.group_id][user_id][i]);
    }
    conf.p->cq_send(oss.str(), conf);
}
void warn_recorder::save()
{
    Json::Value J;
    for (const auto &group_pair : warns) {
        Json::Value group_val;
        for (const auto &user_pair : group_pair.second) {
            Json::Value user_val;
            for (const auto &warn : user_pair.second) {
                user_val.append(warn);
            }
            group_val[std::to_string(user_pair.first)] = user_val;
        }
        J[std::to_string(group_pair.first)] = group_val;
    }
    writefile("./config/warns.json", J.toStyledString());
}
warn_recorder::warn_recorder()
{
    Json::Value J;
    try {
        J = string_to_json(readfile("./config/warns.json", "{}"));
    }
    catch (...) {
        return;
    }
    for (const auto &group_member : J.getMemberNames()) {
        try {
            groupid_t group_id = std::stoull(group_member);
            const Json::Value &group_val = J[group_member];
            for (const auto &user_member : group_val.getMemberNames()) {
                userid_t user_id = std::stoull(user_member);
                const Json::Value &user_val = group_val[user_member];
                for (const auto &warn : user_val) {
                    warns[group_id][user_id].push_back(warn.asString());
                }
            }
        } catch (...) {
            continue;
        }
    }
}

bool warn_recorder::check(std::string message, const msg_meta &conf)
{
    return message.find("/warn") == 0 && is_group_op(conf.p, conf.group_id, conf.user_id);
}

std::string warn_recorder::help()
{
    return "用户警告记录。管理使用/warn qq msg";
}

DECLARE_FACTORY_FUNCTIONS(warn_recorder)