#include "m_change_f.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>
#include <fmt/core.h>

const std::string WELCOME_MESSAGE_FILE = "./config/welcome_messages.json";

m_change_f::m_change_f() {
    load_welcome_messages();
}
m_change_f::~m_change_f() {
    save_welcome_messages();
}

void m_change_f::save_welcome_messages() {
    Json::Value root;
    for (const auto &pair : group_welcome_messages) {
        root[std::to_string(pair.first)] = wstring_to_string(pair.second);
    }
    root["default"] = wstring_to_string(default_welcome_message);

    writefile(WELCOME_MESSAGE_FILE, root.toStyledString());
}
void m_change_f::load_welcome_messages(){
    Json::Value root = string_to_json(readfile(WELCOME_MESSAGE_FILE, "{\"default\": \"\"}"));
    for (const auto &group : root.getMemberNames()) {
        try {
            groupid_t group_id = std::stoull(group);
            group_welcome_messages[group_id] = string_to_wstring(root[group].asString());
        } catch (const std::exception &e) {
            ;
        }
    }
    default_welcome_message = string_to_wstring(root["default"].asString());
}
std::wstring m_change_f::get_welcome_message(groupid_t group_id){
    if (group_welcome_messages.find(group_id) != group_welcome_messages.end()) {
        return group_welcome_messages[group_id];
    }
    return default_welcome_message;
}
std::wstring m_change_f::format_message(const std::wstring &message, const msg_meta &conf) {
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
    std::wstring message_w = string_to_wstring(message);
    if (conf.message_type == "internal") {
        std::wstring welcome_message = trim(this->format_message(this->get_welcome_message(conf.group_id), conf));
        if (!welcome_message.empty()) {
            conf.p->cq_send(wstring_to_string(welcome_message),
                (msg_meta){std::string("group"), conf.user_id, conf.group_id, conf.message_id, conf.p});
            conf.p->setlog(LOG::INFO, fmt::format("{} 入群消息: {}", conf.group_id, wstring_to_string(welcome_message)));
        }
    } else {
        if (conf.p->is_op(conf.user_id) == false && is_group_op(conf.p, conf.group_id, conf.user_id) == false) {
            conf.p->cq_send("你没有权限设置入群消息", conf);
            return;
        }
        if (message_w.find(L"设置入群消息") == 0) {
            std::wstring new_msg = trim(message_w.substr(6));
            this->group_welcome_messages[conf.group_id] = new_msg;
            this->save_welcome_messages();
            conf.p->setlog(LOG::INFO, fmt::format("{} 设置入群消息: {}", conf.group_id, wstring_to_string(new_msg)));
            std::wstring test_msg = trim(this->format_message(this->get_welcome_message(conf.group_id), conf));
            conf.p->cq_send(fmt::format("设置入群消息成功!\n{}", wstring_to_string(test_msg)), conf);
        } else if (message_w.find(L"删除入群消息") == 0) {
            group_welcome_messages[conf.group_id] = L"";
            this->save_welcome_messages();
            conf.p->setlog(LOG::INFO, fmt::format("{} 删除入群消息", conf.group_id));
            conf.p->cq_send("删除入群消息成功", conf);
            return;
        } else {
            conf.p->cq_send("未知命令，请使用 '设置入群消息 [消息内容]'", conf);
        }
    }
}
bool m_change_f::check(std::string message, const msg_meta &conf)
{
    return conf.group_id != 0 && (
        message.find("设置入群消息") == 0
        || (conf.message_type == "internal" && message == "m_change_f")
        || message.find("删除入群消息") == 0);
}
std::string m_change_f::help() { return "入群提示词。\n    设置入群消息 [后接入群提示消息，{{username}}代表用户名]\n    删除入群消息"; }

DECLARE_FACTORY_FUNCTIONS(m_change_f)