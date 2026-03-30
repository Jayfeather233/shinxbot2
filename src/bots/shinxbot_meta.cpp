#include "shinxbot.hpp"

#include <filesystem>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>

namespace fs = fs;

bool shinxbot::meta_func(std::string message, const msg_meta &conf)
{
    std::string normalized = trim(message);
    const std::string at_me = "[CQ:at,qq=" + std::to_string(get_botqq()) + "]";

    const bool is_start_with_at_me = normalized.rfind(at_me, 0) == 0;
    if (is_start_with_at_me) {
        normalized = trim(normalized.substr(normalized.find(']') + 1));
    }

    auto can_manage_group = [&]() {
        return is_group_op(conf.p, conf.group_id, conf.user_id) ||
               is_op(conf.user_id);
    };

    auto handle_bot_help = [&]() {
        help_level_t help_level = help_level_t::public_only;
        if (conf.message_type == "private" && is_op(conf.user_id)) {
            help_level = help_level_t::bot_admin;
        }
        else if (conf.message_type == "group" &&
                 is_group_op(conf.p, conf.group_id, conf.user_id)) {
            help_level = help_level_t::group_admin;
        }
        std::string help_message;
        for (auto funcx : functions) {
            processable *func = std::get<0>(funcx);
            std::string name = std::get<2>(funcx);
            if (conf.message_type == "group" &&
                group_blocklist[conf.group_id].is_blocked(name)) {
                continue;
            }
            std::string h = func->help(conf, help_level);
            if (!trim(h).empty())
                help_message += h + '\n';
        }
        help_message += "本Bot项目地址：https://github.com/"
                        "Jayfeather233/shinxbot2";
        cq_send(help_message, conf);
        return false;
    };

    auto handle_bot_op_help = [&]() {
        if (conf.message_type != "private") {
            cq_send("OP-only help is only available in private chat.", conf);
            return false;
        }
        if (!is_op(conf.user_id)) {
            cq_send("Only OP can use bot.op_help", conf);
            return false;
        }

        std::string help_message;
        help_message += "-----OP core commands-----\n"
                        "bot.on\n"
                        "bot.off\n"
                        "bot.backup\n"
                        "bot.reload function [name|all]\n"
                        "bot.load [function|event] name\n"
                        "bot.unload [function|event] name\n"
                        "bot.list_alias\n"
                        "bot.list_module\n"
                        "-----Feature commands-----";

        for (auto funcx : functions) {
            processable *func = std::get<0>(funcx);
            std::string op_help =
                trim(func->help(conf, help_level_t::bot_admin));
            if (op_help.empty()) {
                continue;
            }

            std::string public_help =
                trim(func->help(conf, help_level_t::public_only));
            if (!public_help.empty() && op_help == public_help) {
                continue;
            }

            if (!trim(op_help).empty()) {
                help_message += op_help + '\n';
            }
        }

        if (trim(help_message).empty()) {
            help_message = "No OP-only help available.";
        }
        cq_send(help_message, conf);
        return false;
    };

    auto handle_bot_off = [&]() {
        bot_enabled = false;
        cq_send("isopen=" + std::to_string(bot_enabled), conf);
        return false;
    };

    auto handle_bot_on = [&]() {
        bot_enabled = true;
        cq_send("isopen=" + std::to_string(bot_enabled), conf);
        return false;
    };

    auto handle_bot_backup = [&]() {
        std::time_t nt = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        tm tt = *localtime(&nt);
        std::ostringstream oss;
        oss << "./backup/" << std::put_time(&tt, "%Y-%m-%d_%H-%M-%S") << ".zip";

        if (!fs::exists("./backup")) {
            fs::create_directories("./backup");
        }

        this->archive->make_archive(oss.str());

        std::string filepa = fs::absolute(oss.str()).string();
        if (conf.message_type == "private") {
            send_file_private(conf.p, conf.user_id, filepa);
        }
        else {
            upload_file(conf.p, filepa, conf.group_id, "backup");
        }
        return false;
    };

    auto handle_bot_list_module = [&]() {
        std::ostringstream oss;
        oss << "functions:\n";
        for (auto u : functions) {
            oss << "  " << std::get<2>(u) << std::endl;
        }
        oss << "events:\n";
        for (auto u : events) {
            oss << "  " << std::get<2>(u) << std::endl;
        }
        cq_send(oss.str(), conf);
        return false;
    };

    auto handle_bot_list_alias = [&]() {
        auto fn_names = list_available_module_names(false);
        auto ev_names = list_available_module_names(true);

        std::set<std::string> loaded_fn;
        std::set<std::string> loaded_ev;
        for (const auto &u : functions) {
            loaded_fn.insert(std::get<2>(u));
        }
        for (const auto &u : events) {
            loaded_ev.insert(std::get<2>(u));
        }

        std::vector<std::string> loaded_fn_names;
        std::vector<std::string> unloaded_fn_names;
        for (const auto &n : fn_names) {
            if (loaded_fn.find(n) != loaded_fn.end()) {
                loaded_fn_names.push_back(n);
            }
            else {
                unloaded_fn_names.push_back(n);
            }
        }

        std::vector<std::string> loaded_ev_names;
        std::vector<std::string> unloaded_ev_names;
        for (const auto &n : ev_names) {
            if (loaded_ev.find(n) != loaded_ev.end()) {
                loaded_ev_names.push_back(n);
            }
            else {
                unloaded_ev_names.push_back(n);
            }
        }

        std::ostringstream oss;
        oss << "function aliases(" << fn_names.size() << "):\n";
        oss << "  loaded(" << loaded_fn_names.size() << "):\n";
        for (const auto &n : loaded_fn_names) {
            oss << "    " << n << "\n";
        }
        oss << "  unloaded(" << unloaded_fn_names.size() << "):\n";
        for (const auto &n : unloaded_fn_names) {
            oss << "    " << n << "\n";
        }

        oss << "event aliases(" << ev_names.size() << "):\n";
        oss << "  loaded(" << loaded_ev_names.size() << "):\n";
        for (const auto &n : loaded_ev_names) {
            oss << "    " << n << "\n";
        }
        oss << "  unloaded(" << unloaded_ev_names.size() << "):\n";
        for (const auto &n : unloaded_ev_names) {
            oss << "    " << n << "\n";
        }

        cq_send(trim(oss.str()), conf);
        return false;
    };

    auto handle_bot_progress = [&]() {
        cq_send(descBar(), conf);
        return false;
    };

    auto handle_group_blockclear = [&]() {
        if (can_manage_group()) {
            group_blocklist[conf.group_id].clear();
            cq_send("已清除屏蔽功能", conf);
            save_blocklist();
            return false;
        }
        else {
            return true;
        }
    };

    auto handle_group_block = [&]() {
        if (can_manage_group()) {
            std::istringstream iss(message.substr(9));
            std::string type;
            while (iss >> type) {
                group_blocklist[conf.group_id].add_block(type);
            }
            cq_send("已添加屏蔽功能", conf);
            save_blocklist();
            return false;
        }
        else {
            return true;
        }
    };

    auto handle_group_unblock = [&]() {
        if (can_manage_group()) {
            std::istringstream iss(message.substr(11));
            std::string type;
            while (iss >> type) {
                group_blocklist[conf.group_id].remove_block(type);
            }
            cq_send("已移除屏蔽功能", conf);
            save_blocklist();
            return false;
        }
        else {
            return true;
        }
    };

    auto handle_group_white = [&]() {
        if (can_manage_group()) {
            std::istringstream iss(message.substr(9));
            std::string type;
            while (iss >> type) {
                group_blocklist[conf.group_id].add_white(type);
            }
            cq_send("已添加白名单功能", conf);
            save_blocklist();
            return false;
        }
        else {
            return true;
        }
    };

    auto handle_group_unwhite = [&]() {
        if (can_manage_group()) {
            std::istringstream iss(message.substr(11));
            std::string type;
            while (iss >> type) {
                group_blocklist[conf.group_id].remove_white(type);
            }
            cq_send("已移除白名单功能", conf);
            save_blocklist();
            return false;
        }
        else {
            return true;
        }
    };

    const std::string cmdline = normalized;
    const auto require_op = [&]() { return is_op(conf.user_id); };
    const auto require_group = [&]() { return conf.message_type == "group"; };
    const auto require_group_at_bot = [&]() {
        return conf.message_type == "group" && is_start_with_at_me;
    };

    const std::vector<cmd_exact_rule> exact_rules = {
        {"bot.help", handle_bot_help, {}},
        {"bot.op_help", handle_bot_op_help, {}},
        {"bot.off", handle_bot_off, {require_op}},
        {"bot.on", handle_bot_on, {require_op}},
        {"bot.backup", handle_bot_backup, {require_op}},
        {"bot.list_module", handle_bot_list_module, {require_op}},
        {"bot.list_alias", handle_bot_list_alias, {require_op}},
        {"bot.progress", handle_bot_progress, {}},
        {"bot.blockclear", handle_group_blockclear, {require_group_at_bot}},
    };

    const std::vector<cmd_prefix_rule> prefix_rules = {
        {"bot.load",
         [&]() { return handle_bot_load(cmdline, conf); },
         {require_op}},
        {"bot.reload",
         [&]() { return handle_bot_reload(cmdline, conf); },
         {require_op}},
        {"bot.unload",
         [&]() { return handle_bot_unload(cmdline, conf); },
         {require_op}},
        {"bot.block ", handle_group_block, {require_group_at_bot}},
        {"bot.unblock ", handle_group_unblock, {require_group_at_bot}},
        {"bot.white ", handle_group_white, {require_group_at_bot}},
        {"bot.unwhite ", handle_group_unwhite, {require_group_at_bot}},
    };

    bool handled = false;
    const bool route_result =
        cmd_try_dispatch(cmdline, exact_rules, prefix_rules, handled);
    if (handled) {
        return route_result;
    }

    return true;
}

void shinxbot::save_blocklist()
{
    Json::Value J_block;
    for (const auto &pair : group_blocklist) {
        groupid_t gid = pair.first;
        J_block[std::to_string(gid)] = pair.second.to_json();
    }
    writefile(bot_config_path(this, "core/blocklist.json"),
              J_block.toStyledString());
}
