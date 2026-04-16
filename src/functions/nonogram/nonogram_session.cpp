#include "nonogram.h"
#include "utils.h"

#include <algorithm>
#include <unordered_set>

static std::string extract_cq_param(const std::string &segment,
                                    const std::string &key)
{
    const std::string token = key + "=";
    size_t pos = segment.find(token);
    if (pos == std::string::npos) {
        return "";
    }
    pos += token.size();
    size_t end = segment.find_first_of(",]", pos);
    if (end == std::string::npos || end <= pos) {
        return "";
    }
    return segment.substr(pos, end - pos);
}

static std::string extract_image_url_from_message(const std::string &message)
{
    size_t img_pos = message.find("[CQ:image");
    if (img_pos == std::string::npos) {
        return "";
    }
    size_t img_end = message.find(']', img_pos);
    if (img_end == std::string::npos) {
        return "";
    }
    std::string seg = message.substr(img_pos, img_end - img_pos + 1);

    std::string url = cq_decode(extract_cq_param(seg, "url"));
    if (!url.empty()) {
        return url;
    }

    std::string file = cq_decode(extract_cq_param(seg, "file"));
    std::string lower = file;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    if (starts_with(lower, "http://") || starts_with(lower, "https://")) {
        return file;
    }
    return "";
}

static std::string extract_image_url_from_message_json(const Json::Value &msg)
{
    if (!msg.isArray()) {
        return "";
    }
    for (const auto &seg : msg) {
        if (!seg.isObject() || seg["type"].asString() != "image" ||
            !seg.isMember("data") || !seg["data"].isObject()) {
            continue;
        }
        const Json::Value &data = seg["data"];
        if (data.isMember("url")) {
            std::string url = cq_decode(data["url"].asString());
            if (!url.empty()) {
                return url;
            }
        }
        if (data.isMember("file")) {
            std::string file = cq_decode(data["file"].asString());
            std::string lower = file;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](char c) {
                               return static_cast<char>(
                                   std::tolower(static_cast<unsigned char>(c)));
                           });
            if (starts_with(lower, "http://") ||
                starts_with(lower, "https://")) {
                return file;
            }
        }
    }
    return "";
}

static int64_t extract_reply_id(const std::string &message)
{
    size_t pos = message.find("[CQ:reply");
    if (pos == std::string::npos) {
        return 0;
    }
    size_t end_pos = message.find(']', pos);
    if (end_pos == std::string::npos) {
        return 0;
    }
    std::string seg = message.substr(pos, end_pos - pos + 1);
    std::string reply_id_str = extract_cq_param(seg, "id");
    if (reply_id_str.empty()) {
        return 0;
    }
    return my_string2int64(reply_id_str);
}

static bool has_suffix(const std::string &s, const std::string &suffix)
{
    if (s.size() < suffix.size()) {
        return false;
    }
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string nonogram::parse_image_url(std::string message)
{
    return extract_image_url_from_message(message);
}

std::string nonogram::parse_guess_image(std::string message,
                                        const msg_meta &conf)
{
    std::string img = parse_image_url(message);
    if (!img.empty()) {
        return img;
    }

    int64_t reply_id = extract_reply_id(message);
    if (reply_id == 0) {
        Json::Value self_req;
        self_req["message_id"] = conf.message_id;
        Json::Value self_json =
            string_to_json(conf.p->cq_send("get_msg", self_req));
        if (self_json.isMember("data") &&
            self_json["data"].isMember("message")) {
            std::string self_message =
                messageArr_to_string(self_json["data"]["message"]);
            reply_id = extract_reply_id(self_message);
        }
    }
    if (reply_id == 0) {
        return "";
    }

    Json::Value j;
    j["message_id"] = reply_id;
    Json::Value res_json = string_to_json(conf.p->cq_send("get_msg", j));
    if (!res_json.isMember("data") || !res_json["data"].isMember("message")) {
        return "";
    }

    std::string from_array =
        extract_image_url_from_message_json(res_json["data"]["message"]);
    if (!from_array.empty()) {
        return from_array;
    }

    std::string reply_message =
        messageArr_to_string(res_json["data"]["message"]);
    return parse_image_url(reply_message);
}

std::string nonogram::ensure_nonogram_dir()
{
    std::string dir = bot_resource_path(nullptr, "nonogram");
    fs::create_directories(dir);
    return dir;
}

std::string nonogram::cache_key_prefix(const msg_meta &conf)
{
    if (conf.message_type == "group") {
        return "g_" + std::to_string(conf.group_id);
    }
    return "u_" + std::to_string(conf.user_id);
}

std::string nonogram::get_cached_puzzle_path(const msg_meta &conf)
{
    if (conf.message_type == "group") {
        auto it = group_puzzle_cache.find(conf.group_id);
        return it == group_puzzle_cache.end() ? "" : it->second;
    }
    auto it = user_puzzle_cache.find(conf.user_id);
    return it == user_puzzle_cache.end() ? "" : it->second;
}

std::string nonogram::get_cached_answer_path(const msg_meta &conf)
{
    if (conf.message_type == "group") {
        auto it = group_answer_cache.find(conf.group_id);
        return it == group_answer_cache.end() ? "" : it->second;
    }
    auto it = user_answer_cache.find(conf.user_id);
    return it == user_answer_cache.end() ? "" : it->second;
}

void nonogram::set_cached_paths(const msg_meta &conf,
                                const std::string &puzzle_path,
                                const std::string &answer_path)
{
    if (conf.message_type == "group") {
        group_puzzle_cache[conf.group_id] = puzzle_path;
        group_answer_cache[conf.group_id] = answer_path;
    }
    else {
        user_puzzle_cache[conf.user_id] = puzzle_path;
        user_answer_cache[conf.user_id] = answer_path;
    }
}

void nonogram::clear_cached_paths(const msg_meta &conf)
{
    std::string puzzle = get_cached_puzzle_path(conf);
    std::string answer = get_cached_answer_path(conf);
    if (!puzzle.empty()) {
        fs::remove(puzzle);
    }
    if (!answer.empty()) {
        fs::remove(answer);
    }

    if (conf.message_type == "group") {
        group_puzzle_cache.erase(conf.group_id);
        group_answer_cache.erase(conf.group_id);
    }
    else {
        user_puzzle_cache.erase(conf.user_id);
        user_answer_cache.erase(conf.user_id);
    }
}

void nonogram::cleanup_orphan_cache_files()
{
    std::unordered_set<std::string> live;
    for (const auto &[_, path] : user_puzzle_cache) {
        if (!path.empty()) {
            live.insert(fs::absolute(fs::path(path)).string());
        }
    }
    for (const auto &[_, path] : group_puzzle_cache) {
        if (!path.empty()) {
            live.insert(fs::absolute(fs::path(path)).string());
        }
    }
    for (const auto &[_, path] : user_answer_cache) {
        if (!path.empty()) {
            live.insert(fs::absolute(fs::path(path)).string());
        }
    }
    for (const auto &[_, path] : group_answer_cache) {
        if (!path.empty()) {
            live.insert(fs::absolute(fs::path(path)).string());
        }
    }

    std::string dir = ensure_nonogram_dir();
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const fs::path p = entry.path();
        const std::string name = p.filename().string();
        const bool is_cache_file =
            has_suffix(name, "_puzzle.png") || has_suffix(name, "_answer.png");
        if (!is_cache_file) {
            continue;
        }
        const std::string abs = fs::absolute(p).string();
        if (live.find(abs) == live.end()) {
            fs::remove(p);
        }
    }
}

bool nonogram::has_pending_guess(const msg_meta &conf) const
{
    auto it = pending_guess_users.find(conf.user_id);
    return it != pending_guess_users.end() && it->second;
}

void nonogram::mark_pending_guess(const msg_meta &conf)
{
    pending_guess_users[conf.user_id] = true;
}

void nonogram::clear_pending_guess(const msg_meta &conf)
{
    pending_guess_users.erase(conf.user_id);
}

bool nonogram::has_pending_debug(const msg_meta &conf) const
{
    auto it = pending_debug_users.find(conf.user_id);
    return it != pending_debug_users.end() && it->second;
}

void nonogram::mark_pending_debug(const msg_meta &conf)
{
    pending_debug_users[conf.user_id] = true;
}

void nonogram::clear_pending_debug(const msg_meta &conf)
{
    pending_debug_users.erase(conf.user_id);
}

bool nonogram::has_active_game(const msg_meta &conf) const
{
    if (conf.message_type == "group") {
        return group_current_levels.find(conf.group_id) !=
               group_current_levels.end();
    }
    return user_current_levels.find(conf.user_id) != user_current_levels.end();
}

std::shared_ptr<nonogram_level> nonogram::get_active_level(const msg_meta &conf)
{
    if (conf.message_type == "group") {
        auto it = group_current_levels.find(conf.group_id);
        return it == group_current_levels.end() ? nullptr : it->second;
    }
    auto it = user_current_levels.find(conf.user_id);
    return it == user_current_levels.end() ? nullptr : it->second;
}

void nonogram::set_active_level(const msg_meta &conf,
                                std::shared_ptr<nonogram_level> level)
{
    if (conf.message_type == "group") {
        group_current_levels[conf.group_id] = level;
    }
    else {
        user_current_levels[conf.user_id] = level;
    }
}

void nonogram::clear_active_level(const msg_meta &conf)
{
    if (conf.message_type == "group") {
        group_current_levels.erase(conf.group_id);
    }
    else {
        user_current_levels.erase(conf.user_id);
    }
}
