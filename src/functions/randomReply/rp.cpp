#include "rp.hpp"

#include "utils.h"

static std::string config_path = "./config/randomReply.json";

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
    return message.find("rp.") == 0 ||
           reply_content.find(conf.user_id) != reply_content.end();
}

void RP::process(std::string message, const msg_meta &conf)
{
    if (message.find("rp.del ") == 0 &&
        (is_group_op(conf.p, conf.group_id, conf.user_id) ||
         conf.p->is_op(conf.user_id))) {
        size_t space1 = message.find(' ', 7);
        if (space1 == std::string::npos) {
            conf.p->cq_send(
                "Invalid command format. Use: rp.add userid possibility "
                "message, where possibility is in percentage",
                conf);
            return;
        }
        userid_t user_id = std::stoll(message.substr(7, space1 - 7));
        reply_content.erase(user_id);
        conf.p->cq_send("Reply removed successfully.", conf);
        save();
    } else if (message.find("rp.add ") == 0 &&
        (is_group_op(conf.p, conf.group_id, conf.user_id) ||
         conf.p->is_op(conf.user_id))) {
        // Command format: rp.add userid possi message
        size_t space1 = message.find(' ', 7);
        if (space1 == std::string::npos) {
            conf.p->cq_send(
                "Invalid command format. Use: rp.add userid possibility "
                "message, where possibility is in percentage",
                conf);
            return;
        }

        size_t space2 = message.find(' ', space1 + 1);
        if (space2 == std::string::npos) {
            conf.p->cq_send(
                "Invalid command format. Use: rp.add userid possibility "
                "message, where possibility is in percentage",
                conf);
            return;
        }

        userid_t user_id = std::stoll(message.substr(7, space1 - 7));
        uint32_t possibility =
            std::stoul(message.substr(space1 + 1, space2 - space1 - 1));
        if (possibility > 100) {
            conf.p->cq_send("Possibility must be between 0 and 100.", conf);
            return;
        }

        std::string reply_msg = trim(message.substr(space2 + 1));

        reply_content[user_id] = std::make_pair(possibility, reply_msg);
        save();
        conf.p->cq_send("Reply added successfully.", conf);
    }
    else if (message == "rp.list") {
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
                    "User ID: {} | Probability: {}% | Message: {}\n", user_id,
                    content.first, content.second);
            }
        }
        else {
            response = "You do not have permission to view this list.";
        }

        if (response.empty() ||
            response == "Replies for users in this group:\n") {
            ;
        }
        else {
            conf.p->cq_send(response, conf);
        }
    }

    else {
        auto it = reply_content.find(conf.user_id);
        if (it != reply_content.end() && get_random(100) < it->second.first) {
            conf.p->cq_send(it->second.second, conf);
        }
    }
}

std::string RP::help()
{
    return "Automatic Reply Bot:\n"
           "- Add a reply: rp.add <userid> <message>\n"
           "- Replies are triggered automatically when a message is received "
           "from the specified user.";
}

DECLARE_FACTORY_FUNCTIONS(RP)
