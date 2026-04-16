#include "nonogram.h"
#include "utils.h"

#include <cctype>
#include <mutex>
#include <sstream>

static std::mutex data_rw;

static std::string nonogram_help_by_level(help_level_t level)
{
    std::string msg = "nonogram游戏，输入 *nonogram [1~3] 开始游戏\n"
                      "在线题库：*nonogram.online [0~4]（0:5x5, 1:10x10, "
                      "2:15x15, 3:20x20, 4:25x25）\n"
                      "提交答案：*nonogram.guess + 图片（或回复图片）\n"
                      "调试识别：*nonogram.debug + 图片（或回复图片）\n"
                      "放弃并揭示：*nonogram.giveup";
    if (level == help_level_t::bot_admin) {
        msg += "\n管理命令：*nonogram.reload";
    }
    return msg;
}

static void split_cmd_args(const std::string &body, std::string &cmd,
                           std::string &args)
{
    std::istringstream iss(trim(body));
    iss >> cmd;
    std::getline(iss, args);
    args = trim(args);
}

static std::string strip_leading_cq_segments(std::string s)
{
    s = trim(s);
    while (starts_with(s, "[CQ:")) {
        size_t end = s.find(']');
        if (end == std::string::npos) {
            break;
        }
        s = trim(s.substr(end + 1));
    }
    return s;
}

static bool is_nonogram_command_message(const std::string &message)
{
    std::string body = strip_leading_cq_segments(message);
    return starts_with(body, "*nonogram");
}

static help_level_t resolve_help_level(const msg_meta &conf)
{
    if (conf.p->is_op(conf.user_id)) {
        return help_level_t::bot_admin;
    }
    if (conf.message_type == "group" &&
        is_group_op(conf.p, conf.group_id, conf.user_id)) {
        return help_level_t::group_admin;
    }
    return help_level_t::public_only;
}

void nonogram::load()
{
    Json::Value J;
    try {
        J = string_to_json(readfile(
            bot_config_path(nullptr, "features/nonogram/nonogram.json"), "[]"));
    }
    catch (...) {
        set_global_log(LOG::WARNING, "Failed to load nonogram levels");
        return;
    }

    for (const Json::Value &level : J) {
        if (!level.isMember("data") || !level["data"].isArray() ||
            level["data"].empty()) {
            continue;
        }
        std::vector<std::vector<bool>> data;
        size_t row_width = 0;
        bool valid_level = true;

        for (const Json::Value &row : level["data"]) {
            if (!row.isArray()) {
                valid_level = false;
                break;
            }
            std::vector<bool> row_data;
            for (const Json::Value &cell : row) {
                row_data.push_back(cell.asBool());
            }
            if (row_width == 0) {
                row_width = row_data.size();
            }
            if (row_data.empty() || row_data.size() != row_width) {
                valid_level = false;
                break;
            }
            data.push_back(row_data);
        }

        if (!valid_level || data.empty() || row_width == 0) {
            continue;
        }
        levels.emplace_back(data, level["pic"].asString());
    }
}

nonogram::nonogram()
{
    std::lock_guard<std::mutex> lock(data_rw);
    load();
}

void nonogram::reload()
{
    std::lock_guard<std::mutex> lock(data_rw);

    for (const auto &u : user_puzzle_cache) {
        fs::remove(u.second);
    }
    for (const auto &g : group_puzzle_cache) {
        fs::remove(g.second);
    }
    for (const auto &u : user_answer_cache) {
        fs::remove(u.second);
    }
    for (const auto &g : group_answer_cache) {
        fs::remove(g.second);
    }

    user_current_levels.clear();
    group_current_levels.clear();
    user_puzzle_cache.clear();
    group_puzzle_cache.clear();
    user_answer_cache.clear();
    group_answer_cache.clear();
    levels.clear();
    load();
}

bool nonogram::reload(const msg_meta &conf)
{
    (void)conf;
    reload();
    return true;
}

void nonogram::process_command(std::string message, const msg_meta &conf)
{
    const std::string normalized = strip_leading_cq_segments(message);
    std::string body;
    if (normalized == "*nonogram") {
        body.clear();
    }
    else if (!cmd_strip_prefix(normalized, "*nonogram.", body) &&
             !cmd_strip_prefix(normalized, "*nonogram ", body)) {
        return;
    }

    std::string cmd;
    std::string args;
    split_cmd_args(body, cmd, args);

    const std::vector<cmd_exact_rule> exact_rules = {
        {"help",
         [&]() {
             conf.p->cq_send(nonogram_help_by_level(resolve_help_level(conf)),
                             conf);
             return true;
         }},
        {"reload",
         [&]() {
             if (!conf.p->is_op(conf.user_id)) {
                 return true;
             }
             reload();
             conf.p->cq_send("关卡已重新加载", conf);
             return true;
         }},
        {"quit",
         [&]() {
             clear_active_level(conf);
             clear_cached_paths(conf);
             clear_pending_guess(conf);
             clear_pending_debug(conf);
             cleanup_orphan_cache_files();
             conf.p->cq_send("游戏已退出", conf);
             return true;
         }},
        {"giveup", [&]() { return handle_giveup(conf); }},
    };

    const std::vector<cmd_prefix_rule> prefix_rules = {
        {"online", [&]() { return handle_online_start(args, conf); }},
        {"guess", [&]() { return handle_guess(normalized, conf); }},
        {"debug", [&]() { return handle_debug(normalized, conf); }},
    };

    bool handled = false;
    (void)cmd_try_dispatch(cmd, exact_rules, prefix_rules, handled);
    if (handled) {
        return;
    }

    if (body.empty() || std::isdigit(static_cast<unsigned char>(body[0]))) {
        (void)handle_local_start(body, conf);
        return;
    }

    conf.p->cq_send("未知命令，输入 *nonogram.help 查看用法", conf);
}

void nonogram::process(std::string message, const msg_meta &conf)
{
    if (has_pending_debug(conf)) {
        std::string img = parse_image_url(message);
        if (!img.empty() && trim(message).find("[CQ:image") == 0) {
            (void)handle_debug(message, conf);
            return;
        }
    }

    if (has_pending_guess(conf)) {
        std::string img = parse_image_url(message);
        if (!img.empty() && trim(message).find("[CQ:image") == 0) {
            (void)handle_guess(message, conf);
            return;
        }
    }

    if (is_nonogram_command_message(message)) {
        process_command(message, conf);
    }
}

bool nonogram::check(std::string message, const msg_meta &conf)
{
    if (is_nonogram_command_message(message)) {
        return true;
    }
    if (has_pending_debug(conf)) {
        std::string img = parse_image_url(message);
        return !img.empty() && trim(message).find("[CQ:image") == 0;
    }
    if (has_pending_guess(conf)) {
        std::string img = parse_image_url(message);
        return !img.empty() && trim(message).find("[CQ:image") == 0;
    }
    return false;
}

std::string nonogram::help()
{
    return "nonogram 游戏。输入 *nonogram.help 查看详细帮助。";
}

std::string nonogram::help(const msg_meta &conf, help_level_t level)
{
    (void)conf;
    (void)level;
    return "nonogram 游戏。输入 *nonogram.help 查看详细帮助。";
}

DECLARE_FACTORY_FUNCTIONS(nonogram)
