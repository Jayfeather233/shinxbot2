#include "auto_approve.h"
#include "internal_message.hpp"
#include "utils.h"

namespace {
const std::string AUTO_APPROVE_CONFIG_FILE =
    bot_config_path(nullptr, "features/auto_approve/auto_approve.json");

bool is_approve_command(const std::string &m)
{
    return cmd_match_exact(m, {"approve", "approve.help", "approve.status"}) ||
           cmd_match_prefix(m, {"approve.friend", "approve.invite"});
}

std::string approve_usage()
{
    return "Approve controls (OP only):\n"
           "approve.help\n"
           "approve.status\n"
           "approve.friend on|off\n"
           "approve.invite on|off";
}
} // namespace

auto_approve::auto_approve() { load_config(); }

void auto_approve::load_config()
{
    Json::Value cfg = string_to_json(readfile(AUTO_APPROVE_CONFIG_FILE, "{}"));
    if (!cfg.isObject()) {
        auto_friend_ = true;
        auto_group_invite_ = true;
        return;
    }

    auto_friend_ =
        cfg.isMember("auto_friend") ? cfg["auto_friend"].asBool() : true;
    auto_group_invite_ = cfg.isMember("auto_group_invite")
                             ? cfg["auto_group_invite"].asBool()
                             : true;
}

void auto_approve::save_config() const
{
    Json::Value cfg(Json::objectValue);
    cfg["auto_friend"] = auto_friend_;
    cfg["auto_group_invite"] = auto_group_invite_;
    writefile(AUTO_APPROVE_CONFIG_FILE, cfg.toStyledString());
}

bool auto_approve::parse_switch_value(const std::string &value, bool &state)
{
    std::string v = trim(value);
    if (v == "on" || v == "true" || v == "1") {
        state = true;
        return true;
    }
    if (v == "off" || v == "false" || v == "0") {
        state = false;
        return true;
    }
    return false;
}

std::string auto_approve::state_to_text(bool state) const
{
    return state ? "on" : "off";
}

void auto_approve::process(std::string message, const msg_meta &conf)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::string m = trim(message);

    if (conf.message_type == "internal") {
        if (conf.message_id == internal_message::kApproveFriendRequest) {
            if (!auto_friend_) {
                return;
            }
            std::string flag = m;
            if (flag.empty()) {
                return;
            }

            Json::Value res;
            res["flag"] = flag;
            res["sub_type"] = "add";
            res["approve"] = true;
            conf.p->cq_send("set_friend_add_request", res);
            conf.p->setlog(LOG::INFO, "friend add approved by " +
                                          std::to_string(conf.user_id));
            return;
        }

        if (conf.message_id == internal_message::kApproveGroupInvite) {
            if (!auto_group_invite_) {
                return;
            }
            std::string flag = m;
            if (flag.empty()) {
                return;
            }

            Json::Value res;
            res["flag"] = flag;
            res["sub_type"] = "invite";
            res["approve"] = true;
            conf.p->cq_send("set_group_add_request", res);
            conf.p->setlog(LOG::INFO, "group invite approved in group " +
                                          std::to_string(conf.group_id) +
                                          " by " +
                                          std::to_string(conf.user_id));
            return;
        }

        return;
    }

    auto set_switch = [&](const std::string &raw_value, bool &field,
                          const std::string &label) {
        bool state = false;
        if (!parse_switch_value(raw_value, state)) {
            conf.p->cq_send("Invalid value. Use: on|off", conf);
            return;
        }
        field = state;
        save_config();
        conf.p->cq_send(label + " set to: " + state_to_text(state), conf);
    };

    auto require_op = [&]() { return conf.p->is_op(conf.user_id); };

    const std::vector<cmd_exact_rule> exact_rules = {
        {"approve",
         [&]() {
             conf.p->cq_send(approve_usage(), conf);
             return true;
         },
         {require_op}},
        {"approve.help",
         [&]() {
             conf.p->cq_send(approve_usage(), conf);
             return true;
         },
         {require_op}},
        {"approve.status",
         [&]() {
             conf.p->cq_send(
                 "auto friend: " + state_to_text(auto_friend_) +
                     "\nauto invite: " + state_to_text(auto_group_invite_),
                 conf);
             return true;
         },
         {require_op}},
    };

    const std::vector<cmd_prefix_rule> prefix_rules = {
        {"approve.friend",
         [&]() {
             std::string body;
             if (cmd_strip_prefix(m, "approve.friend", body)) {
                 set_switch(body, auto_friend_, "auto friend");
             }
             return true;
         },
         {require_op}},
        {"approve.invite",
         [&]() {
             std::string body;
             if (cmd_strip_prefix(m, "approve.invite", body)) {
                 set_switch(body, auto_group_invite_, "auto invite");
             }
             return true;
         },
         {require_op}},
    };

    if (cmd_dispatch(m, exact_rules, prefix_rules)) {
        return;
    }

    conf.p->cq_send(approve_usage(), conf);
}

bool auto_approve::check(std::string message, const msg_meta &conf)
{
    std::string m = trim(message);
    if (conf.message_type == "internal") {
        (void)m;
        return conf.message_id == internal_message::kApproveFriendRequest ||
               conf.message_id == internal_message::kApproveGroupInvite;
    }
    return is_approve_command(m);
}

bool auto_approve::reload(const msg_meta &conf)
{
    (void)conf;
    std::unique_lock<std::mutex> lock(mutex_);
    load_config();
    return true;
}

std::string auto_approve::help() { return ""; }

std::string auto_approve::help(const msg_meta &conf, help_level_t level)
{
    if (level != help_level_t::bot_admin || conf.message_type != "private" ||
        !conf.p->is_op(conf.user_id)) {
        return "";
    }

    return "approve: OP-only auto approve controls. Use approve.help for full "
           "usage.";
}

DECLARE_FACTORY_FUNCTIONS(auto_approve)
