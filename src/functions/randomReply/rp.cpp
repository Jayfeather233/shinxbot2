#include "rp.hpp"

#include "utils.h"

#include <sstream>

static std::string config_path =
    bot_config_path(nullptr, "features/randomReply/randomReply.json");

RP::RP()
{
    Json::Value Ja = string_to_json(readfile(config_path, "[]"));
    for (const auto &J : Ja) {
        reply_content.emplace(
            J["userid"].asInt64(),
            std::make_pair(J["poss"].asUInt(), J["msg"].asString()));
    }
}

void RP::save()
{
    Json::Value Ja(Json::arrayValue);
    for (const auto &[user_id, content] : reply_content) {
        Json::Value J;
        J["userid"] = (Json::Int64)user_id;
        J["poss"] = content.first;
        J["msg"] = content.second;
        Ja.append(J);
    }
    writefile(config_path, Ja.toStyledString(), false);
}

bool RP::check(std::string message, const msg_meta &conf)
{
    return cmd_match_prefix(message, {"rp."}) ||
           reply_content.find(conf.user_id) != reply_content.end();
}

bool RP::reload(const msg_meta &conf)
{
    (void)conf;
    reply_content.clear();
    Json::Value Ja = string_to_json(readfile(config_path, "[]"));
    for (const auto &J : Ja) {
        reply_content.emplace(
            J["userid"].asInt64(),
            std::make_pair(J["poss"].asUInt(), J["msg"].asString()));
    }
    return true;
}

void RP::process(std::string message, const msg_meta &conf)
{
    const std::string normalized = trim(message);
    const bool can_manage = is_group_op(conf.p, conf.group_id, conf.user_id) ||
                            conf.p->is_op(conf.user_id);

    std::istringstream iss(normalized);
    std::string cmd;
    iss >> cmd;
    std::string args;
    getline(iss, args);
    args = trim(args);

    const std::vector<cmd_exact_rule> exact_rules = {
        {"rp.del",
         [&]() {
             if (!can_manage) {
                 return true;
             }
             userid_t user_id = my_string2uint64(args);
             if (user_id == 0) {
                 conf.p->cq_send("Invalid command format. Use: rp.del <userid>",
                                 conf);
                 return true;
             }
             reply_content.erase(user_id);
             conf.p->cq_send("Reply removed successfully.", conf);
             save();
             return true;
         }},
        {"rp.add",
         [&]() {
             if (!can_manage) {
                 return true;
             }

             std::istringstream arg_iss(args);
             std::string uid_s, possibility_s;
             if (!(arg_iss >> uid_s >> possibility_s)) {
                 conf.p->cq_send(
                     "Invalid command format. Use: rp.add userid possibility "
                     "message, where possibility is in percentage",
                     conf);
                 return true;
             }

             userid_t user_id = my_string2uint64(uid_s);
             uint32_t possibility = my_string2uint64(possibility_s);
             std::string reply_msg;
             getline(arg_iss, reply_msg);
             reply_msg = trim(reply_msg);

             if (user_id == 0 || reply_msg.empty()) {
                 conf.p->cq_send(
                     "Invalid command format. Use: rp.add userid possibility "
                     "message, where possibility is in percentage",
                     conf);
                 return true;
             }
             if (possibility > 100) {
                 conf.p->cq_send("Possibility must be between 0 and 100.",
                                 conf);
                 return true;
             }

             reply_content[user_id] = std::make_pair(possibility, reply_msg);
             save();
             conf.p->cq_send("Reply added successfully.", conf);
             return true;
         }},
        {"rp.list",
         [&]() {
             std::string response;

             if (conf.message_type == "group") {
                 // Filter and list replies for users in the group
                 response = "Replies for users in this group:\n";
                 for (const auto &[user_id, content] : reply_content) {
                     if (is_group_member(conf.p, conf.group_id, user_id)) {
                         response += fmt::format(
                             "User ID: {} | Probability: {}% | Message: {}\n",
                             user_id, content.first, content.second);
                     }
                 }
             }
             else if (conf.p->is_op(conf.user_id)) {
                 // List all replies (admin access)
                 response = "All configured replies:\n";
                 for (const auto &[user_id, content] : reply_content) {
                     response += fmt::format(
                         "User ID: {} | Probability: {}% | Message: {}\n",
                         user_id, content.first, content.second);
                 }
             }
             else {
                 response = "You do not have permission to view this list.";
             }

             if (!response.empty() &&
                 response != "Replies for users in this group:\n") {
                 conf.p->cq_send(response, conf);
             }
             return true;
         }},
    };

    bool handled = false;
    (void)cmd_try_dispatch(cmd, exact_rules, {}, handled);
    if (handled) {
        return;
    }

    auto it = reply_content.find(conf.user_id);
    if (it != reply_content.end() && get_random(100) < it->second.first) {
        conf.p->cq_send(it->second.second, conf);
    }
}

std::string RP::help()
{
    return "Automatic Reply Bot:\n"
           "- Replies are triggered automatically when a message is received "
           "from the specified user.";
}

std::string RP::help(const msg_meta &conf, help_level_t level)
{
    if (level == help_level_t::group_admin && conf.message_type == "group" &&
        is_group_op(conf.p, conf.group_id, conf.user_id)) {
        return "Automatic Reply Bot:\n"
               "- Add a reply: rp.add <userid> <possibility in %> <message>\n"
               "- Delete a reply: rp.del <userid>\n"
               "- List replies: rp.list\n"
               "- Replies are triggered automatically when a message is "
               "received from the specified user.";
    }

    return help();
}

DECLARE_FACTORY_FUNCTIONS(RP)
