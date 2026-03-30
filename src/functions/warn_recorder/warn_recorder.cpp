#include "warn_recorder.h"
#include "utils.h"

#include <iostream>

void warn_recorder::process_command(std::string command, const msg_meta &conf)
{
    command = trim(command);
    std::istringstream iss(command);
    std::string subcmd;
    iss >> subcmd;
    std::string args;
    getline(iss, args);
    args = trim(args);

    const std::vector<cmd_exact_rule> exact_rules = {
        {"clear",
         [&]() {
             userid_t user_id = my_string2uint64(args);
             if (user_id == 0) {
                 conf.p->cq_send("参数错误。请使用 /warn.clear [qq] 格式",
                                 conf);
                 return true;
             }
             warns[conf.group_id].erase(user_id);
             save();
             conf.p->cq_send("已清除该用户的警告记录", conf);
             return true;
         }},
        {"del",
         [&]() {
             std::string payload = args;
             if (payload.empty()) {
                 conf.p->cq_send("参数错误。请使用 /warn.del [qq] [index] 格式",
                                 conf);
                 return true;
             }
             size_t space_pos = payload.find(' ');
             if (space_pos == std::string::npos) {
                 if (payload[0] != '[') {
                     conf.p->cq_send(
                         "参数错误。请使用 /warn.del [qq] [index] 格式", conf);
                     return true;
                 }
                 space_pos = payload.find(']');
             }
             userid_t user_id = my_string2uint64(payload.substr(0, space_pos));
             if (user_id == 0) {
                 conf.p->cq_send("参数错误。请使用 /warn.del [qq] [index] 格式",
                                 conf);
                 return true;
             }
             size_t index;
             try {
                 index = std::stoul(payload.substr(space_pos + 1)) - 1;
             }
             catch (...) {
                 conf.p->cq_send("参数错误。请使用 /warn.del [qq] [index] 格式",
                                 conf);
                 return true;
             }
             if (index >= warns[conf.group_id][user_id].size()) {
                 conf.p->cq_send("参数错误。索引超出范围", conf);
                 return true;
             }
             warns[conf.group_id][user_id].erase(
                 warns[conf.group_id][user_id].begin() + index);
             save();
             conf.p->cq_send("已删除该条警告记录", conf);
             return true;
         }},
    };

    bool handled = false;
    (void)cmd_try_dispatch(subcmd, exact_rules, {}, handled);
}

// output 已警告 <user> %s 次\n理由: 1. xxx\n2. xxx
void warn_recorder::process(std::string message, const msg_meta &conf)
{
    if (message.size() < 5) {
        conf.p->cq_send("参数错误。请使用 /warn [qq] [msg] 格式", conf);
        return;
    }
    message = trim(message.substr(5));
    if (message.empty()) {
        conf.p->cq_send("参数错误。请使用 /warn [qq] [msg] 格式", conf);
        return;
    }
    if (message[0] == '.') {
        process_command(message.substr(1), conf);
        return;
    }
    size_t space_pos = message.find(' ');
    if (space_pos == std::string::npos) {
        if (message[0] != '[') {
            conf.p->cq_send("参数错误。请使用 /warn [qq] [msg] 格式", conf);
            return;
        }
        else {
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
    oss << fmt::format("已警告 {} {} 次\n",
                       get_username(conf.p, user_id, conf.group_id),
                       warns[conf.group_id][user_id].size());
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
    writefile(bot_config_path(nullptr, "features/warn_recorder/warns.json"),
              J.toStyledString());
}
warn_recorder::warn_recorder() { load_config(); }

void warn_recorder::load_config()
{
    warns.clear();
    Json::Value J;
    try {
        J = string_to_json(readfile(
            bot_config_path(nullptr, "features/warn_recorder/warns.json"),
            "{}"));
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
        }
        catch (...) {
            continue;
        }
    }
}

bool warn_recorder::reload(const msg_meta &conf)
{
    (void)conf;
    load_config();
    return true;
}

bool warn_recorder::check(std::string message, const msg_meta &conf)
{
    return cmd_match_prefix(message, {"/warn"}) &&
           is_group_op(conf.p, conf.group_id, conf.user_id);
}

std::string warn_recorder::help() { return "用户警告记录。"; }

std::string warn_recorder::help(const msg_meta &conf, help_level_t level)
{
    if (level != help_level_t::group_admin || conf.message_type != "group" ||
        !is_group_op(conf.p, conf.group_id, conf.user_id)) {
        return help();
    }

    return "用户警告记录。管理使用 /warn qq msg";
}

DECLARE_FACTORY_FUNCTIONS(warn_recorder)