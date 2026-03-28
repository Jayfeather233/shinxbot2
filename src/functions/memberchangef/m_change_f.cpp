#include "m_change_f.h"
#include "internal_message.hpp"
#include "utils.h"

#include <fmt/core.h>
#include <jsoncpp/json/json.h>

#include <mutex>
#include <vector>

const std::string WELCOME_MESSAGE_FILE =
    bot_config_path("features/memberchangef/welcome_messages.json");

static std::mutex group_mutex;

m_change_f::m_change_f() { load_welcome_messages(); }
m_change_f::~m_change_f() { save_welcome_messages(); }

void m_change_f::save_welcome_messages()
{
    Json::Value root;
    for (const auto &pair : group_welcome_messages) {
        root[std::to_string(pair.first)] = wstring_to_string(pair.second);
    }
    root["default"] = wstring_to_string(default_welcome_message);

    writefile(WELCOME_MESSAGE_FILE, root.toStyledString());
}
void m_change_f::load_welcome_messages()
{
    Json::Value root =
        string_to_json(readfile(WELCOME_MESSAGE_FILE, "{\"default\": \"\"}"));
    for (const auto &group : root.getMemberNames()) {
        try {
            groupid_t group_id = std::stoull(group);
            group_welcome_messages[group_id] =
                string_to_wstring(root[group].asString());
        }
        catch (const std::exception &) {
        }
    }
    default_welcome_message = string_to_wstring(root["default"].asString());
}
std::wstring m_change_f::get_welcome_message(groupid_t group_id)
{
    std::lock_guard<std::mutex> lock(group_mutex);
    if (group_welcome_messages.find(group_id) != group_welcome_messages.end()) {
        return group_welcome_messages[group_id];
    }
    return default_welcome_message;
}
std::wstring m_change_f::format_message(const std::wstring &message,
                                        const msg_meta &conf)
{
    std::wstring formatted_message = message;
    std::string username = get_username(conf.p, conf.user_id, conf.group_id);
    size_t pos = formatted_message.find(L"{{username}}");
    while (pos != std::wstring::npos) {
        formatted_message.replace(pos, 12, string_to_wstring(username));
        pos = formatted_message.find(L"{{username}}");
    }
    return formatted_message;
}

void m_change_f::process(std::string message, const msg_meta &conf)
{
    if (message == "welcome.help") {
        const bool can_manage = conf.p->is_op(conf.user_id) ||
                                is_group_op(conf.p, conf.group_id,
                                            conf.user_id);
        if (!can_manage) {
            conf.p->cq_send("入群欢迎词为管理员功能", conf);
            return;
        }

        conf.p->cq_send("入群欢迎词帮助\n"
                        "welcome.help\n"
                        "设置入群消息 [后接入群提示消息，{{username}}代表用户名]\n"
                        "删除入群消息",
                        conf);
        return;
    }

    std::wstring message_w = string_to_wstring(message);
    if (conf.message_type == "internal") {
        std::wstring welcome_message = trim(this->format_message(
            this->get_welcome_message(conf.group_id), conf));
        if (!welcome_message.empty()) {
            std::lock_guard<std::mutex> lock(group_mutex);
            auto &state = group_state[conf.group_id];
            state.last_join = std::chrono::steady_clock::now();

            // Preserve the old behavior: once a debounce timer is active,
            // only extend its deadline and keep the first queued payload.
            if (state.timer_running) {
                return;
            }

            state.timer_running = true;
            state.welcome_message = welcome_message;
            state.user_id = conf.user_id;
            state.group_id = conf.group_id;
            state.message_id = conf.message_id;
            state.p = conf.p;
        }
    }
    else {
        if (conf.p->is_op(conf.user_id) == false &&
            is_group_op(conf.p, conf.group_id, conf.user_id) == false) {
            conf.p->cq_send("你没有权限设置入群消息", conf);
            return;
        }
        if (message_w.find(L"设置入群消息") == 0) {
            std::wstring new_msg = trim(message_w.substr(6));
            std::lock_guard<std::mutex> lock(group_mutex);
            this->group_welcome_messages[conf.group_id] = new_msg;
            this->save_welcome_messages();
            conf.p->setlog(LOG::INFO,
                           fmt::format("{} 设置入群消息: {}", conf.group_id,
                                       wstring_to_string(new_msg)));
            std::wstring test_msg = trim(this->format_message(
                this->get_welcome_message(conf.group_id), conf));
            conf.p->cq_send(fmt::format("设置入群消息成功!\n{}",
                                        wstring_to_string(test_msg)),
                            conf);
        }
        else if (message_w.find(L"删除入群消息") == 0) {
            std::lock_guard<std::mutex> lock(group_mutex);
            group_welcome_messages[conf.group_id] = L"";
            this->save_welcome_messages();
            conf.p->setlog(LOG::INFO,
                           fmt::format("{} 删除入群消息", conf.group_id));
            conf.p->cq_send("删除入群消息成功", conf);
            return;
        }
        else {
            conf.p->cq_send("未知命令，请使用 '设置入群消息 [消息内容]'", conf);
        }
    }
}

void m_change_f::set_callback(
    std::function<void(std::function<void(bot *p)>)> f)
{
    f([this](bot *p) { flush_welcome_queue(p); });
}

void m_change_f::flush_welcome_queue(bot *p)
{
    struct pending_msg {
        userid_t user_id = 0;
        groupid_t group_id = 0;
        int64_t message_id = 0;
        bot *p = nullptr;
        std::wstring welcome_message;
    };
    std::vector<pending_msg> pending;
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(group_mutex);
        for (auto &it : group_state) {
            auto &state = it.second;
            if (!state.timer_running) {
                continue;
            }
            if (now - state.last_join >= std::chrono::seconds(5)) {
                state.timer_running = false;
                pending.push_back({state.user_id, state.group_id,
                                   state.message_id, state.p,
                                   state.welcome_message});
            }
        }
    }

    for (const auto &item : pending) {
        msg_meta conf{"group", item.user_id, item.group_id, item.message_id,
                      item.p != nullptr ? item.p : p};
        const std::wstring &welcome_message = item.welcome_message;
        if (conf.p != nullptr) {
            conf.p->cq_send(wstring_to_string(welcome_message), conf);
        }
    }
}

bool m_change_f::check(std::string message, const msg_meta &conf)
{
    return conf.group_id != 0 &&
           (message == "welcome.help" || message.find("设置入群消息") == 0 ||
            (conf.message_type == "internal" &&
             conf.message_id == internal_message::kMemberChangeWelcome) ||
            message.find("删除入群消息") == 0);
}
std::string m_change_f::help() { return ""; }

std::string m_change_f::help(const msg_meta &conf, help_level_t level)
{
    if (level == help_level_t::bot_admin) {
        return "入群欢迎词管理（管理员）。帮助：welcome.help";
    }

    if (level == help_level_t::group_admin && conf.message_type == "group" &&
        is_group_op(conf.p, conf.group_id, conf.user_id)) {
        return "入群欢迎词管理（管理员）。帮助：welcome.help";
    }

    return "";
}

DECLARE_FACTORY_FUNCTIONS(m_change_f)