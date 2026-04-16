#include "nonogram.h"
#include "utils.h"

#include <fmt/core.h>

#include <regex>
#include <sstream>

static void nonogram_react_or_reply(const msg_meta &conf,
                                    const std::string &emoji_id,
                                    const std::string &fallback)
{
    bool ok = false;
    try {
        Json::Value j;
        j["message_id"] = conf.message_id;
        j["emoji_id"] = emoji_id;
        Json::Value r =
            string_to_json(conf.p->cq_send("set_msg_emoji_like", j));
        ok = r["status"].asString() == "ok";
    }
    catch (...) {
    }

    if (!ok && !fallback.empty()) {
        conf.p->cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                            "]" + fallback,
                        conf);
    }
}

bool nonogram::send_win_message(std::shared_ptr<nonogram_level> level,
                                const msg_meta &conf)
{
    std::string answer_path = get_cached_answer_path(conf);
    if (answer_path.empty() || !fs::exists(answer_path)) {
        answer_path =
            bot_resource_path(nullptr, "nonogram/answer/" + level->get_pic());
    }

    nonogram_react_or_reply(conf, "9989", "[CQ:face,id=9989]");

    if (!answer_path.empty() && fs::exists(answer_path)) {
        conf.p->cq_send(
            fmt::format(
                "[CQ:at,qq={}] 答案正确！\n[CQ:image,file=file://{},id=40000]",
                conf.user_id, fs::absolute(fs::path(answer_path)).string()),
            conf);
    }
    else {
        conf.p->cq_send(fmt::format("[CQ:at,qq={}] 答案正确！", conf.user_id),
                        conf);
    }
    return true;
}

bool nonogram::start_level_for_session(std::shared_ptr<nonogram_level> level,
                                       const msg_meta &conf,
                                       const std::string &loaded_msg)
{
    cleanup_orphan_cache_files();

    if (has_active_game(conf)) {
        send_puzzle(get_active_level(conf), conf);
        if (conf.message_type == "group") {
            conf.p->cq_send(
                "本群已经有一个正在进行的nonogram游戏了，请完成它或者"
                "等待它结束后再开始新的游戏",
                conf);
        }
        else {
            conf.p->cq_send("你已经有一个正在进行的nonogram游戏了，请完成它或者"
                            "等待它结束后再开始新的游戏",
                            conf);
        }
        return true;
    }

    set_active_level(conf, level);
    send_puzzle(level, conf);
    if (loaded_msg.empty()) {
        conf.p->cq_send("使用 *nonogram.guess 提交答案图片", conf);
    }
    else {
        conf.p->cq_send(loaded_msg + "\n使用 *nonogram.guess 提交答案图片",
                        conf);
    }
    return true;
}

bool nonogram::handle_local_start(const std::string &args, const msg_meta &conf)
{
    if (levels.empty()) {
        conf.p->cq_send("当前没有可用的关卡", conf);
        return true;
    }

    int index = 0;
    if (args.empty()) {
        index = get_random(levels.size());
    }
    else {
        index = my_string2int64(args) - 1;
    }

    if (index < 0 || index >= static_cast<int>(levels.size())) {
        conf.p->cq_send("无效的关卡索引", conf);
        return true;
    }

    auto level = std::make_shared<nonogram_level>(levels[index]);
    return start_level_for_session(level, conf);
}

bool nonogram::handle_online_start(const std::string &args,
                                   const msg_meta &conf)
{
    int size = 1;
    if (!args.empty()) {
        size = my_string2int64(args);
    }
    if (size < 0 || size > 4) {
        conf.p->cq_send("在线题库尺寸参数无效，请使用 0~4", conf);
        return true;
    }

    std::shared_ptr<nonogram_level> level;
    std::string puzzle_id;
    std::string err_msg;
    bool ok = fetch_online_level(size, level, puzzle_id, err_msg, false);
    if (!ok) {
        ok = fetch_online_level(size, level, puzzle_id, err_msg, true);
    }
    if (!ok || !level) {
        conf.p->cq_send("在线题目加载失败：" + err_msg, conf);
        return true;
    }

    conf.p->setlog(LOG::INFO,
                   fmt::format("Loaded online nonogram puzzle {} size {}",
                               puzzle_id, size));
    return start_level_for_session(level, conf,
                                   "已加载在线题目 ID: " + puzzle_id);
}

bool nonogram::handle_guess(const std::string &raw_message,
                            const msg_meta &conf)
{
    auto level = get_active_level(conf);
    if (!level) {
        conf.p->cq_send("当前没有正在进行的nonogram游戏", conf);
        return true;
    }

    std::string img = parse_guess_image(raw_message, conf);
    if (img.empty()) {
        static const std::regex guess_only_re(
            "^\\*nonogram(?:\\.guess|\\s+guess)\\s*$");
        if (std::regex_match(trim(raw_message), guess_only_re)) {
            clear_pending_debug(conf);
            mark_pending_guess(conf);
            conf.p->cq_send("请发送答案图片", conf);
            return true;
        }
        conf.p->cq_send("请在命令中附图，或回复一条图片消息", conf);
        return true;
    }

    clear_pending_guess(conf);

    std::string uuid = generate_uuid();
    std::string nonogram_dir = ensure_nonogram_dir();
    std::string filename = nonogram_dir + "/" + uuid + ".png";
    try {
        download(cq_decode(img), nonogram_dir + "/", uuid + ".png");
    }
    catch (...) {
        conf.p->cq_send("无法下载图片，请检查图片链接是否有效", conf);
        return true;
    }

    bool result = check_game(filename, level, conf);
    fs::remove(filename);
    if (result) {
        send_win_message(level, conf);
        clear_active_level(conf);
        clear_cached_paths(conf);
        cleanup_orphan_cache_files();
    }
    else {
        conf.p->cq_send(
            fmt::format("[CQ:at,qq={}] 答案不正确，请继续努力！", conf.user_id),
            conf);
    }
    return true;
}

bool nonogram::handle_giveup(const msg_meta &conf)
{
    auto level = get_active_level(conf);
    if (!level) {
        conf.p->cq_send("当前没有正在进行的nonogram游戏", conf);
        return true;
    }

    std::string answer_path = get_cached_answer_path(conf);
    if (answer_path.empty() || !fs::exists(answer_path)) {
        answer_path =
            bot_resource_path(nullptr, "nonogram/answer/" + level->get_pic());
    }

    if (!answer_path.empty() && fs::exists(answer_path)) {
        conf.p->cq_send("已放弃本局，答案如下：\n[CQ:image,file=file://" +
                            fs::absolute(fs::path(answer_path)).string() +
                            ",id=40000]",
                        conf);
    }
    else {
        conf.p->cq_send("已放弃本局，但答案图不可用", conf);
    }

    clear_active_level(conf);
    clear_cached_paths(conf);
    clear_pending_guess(conf);
    clear_pending_debug(conf);
    cleanup_orphan_cache_files();
    return true;
}
