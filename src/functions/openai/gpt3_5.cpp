#include "gpt3_5.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <ctime>
#include <cstring>
#include <chrono>

/**
 * Overall API intro: https://platform.openai.com/docs/api-reference/chat/create
 * do post at https://api.openai.com/v1/chat/completions
 * model: gpt-3.5-turbo
 * upload:
 * {
 *  "model": "gpt-3.5-turbo",
 *  "messages": [{"role": "user", "content": "Hello!"}]
 * }
 * messages format: https://platform.openai.com/docs/guides/chat/introduction
 * temperature: 0~1
 * max_tokens: default inf(4096)
 *
 * ATTENTION!!!
 * In general, gpt-3.5-turbo-0301 does not pay strong attention to the system
 * message, and therefore important instructions are often better placed in a
 * user message.
 */

int MAX_TOKEN = 4000;
int MAX_REPLY = 1000;
int RED_LINE = 1000;
const int MAX_KEYS = 255;

std::mutex gptlock[MAX_KEYS];

gpt3_5::gpt3_5()
{
    const std::string openai_conf_path =
        bot_config_path(nullptr, "features/openai/openai.json");
    const std::string gpt_history_dir = bot_config_path(nullptr, "gpt3_5");

    if (!fs::exists(openai_conf_path)) {
        std::cout << "Please config your openai key in openai.json (and "
                     "restart) OR see openai_example.json"
                  << std::endl;
        std::ofstream of(openai_conf_path, std::ios::out);
        of << "{"
              "\"keys\": [\"\"],"
              "\"mode\": [\"default\"],"
              "\"default\": [{\"role\": \"system\", \"content\": \"You are a "
              "helpful assistant.\"}],"
              "\"black_list\": [\"股票\"],"
              "\"MAX_TOKEN\": 4000,"
              "\"MAX_REPLY\": 700,"
              "\"RED_LINE\": 1000"
              "}";
        of.close();
    }

    std::string ans = readfile(openai_conf_path);

    Json::Value res = string_to_json(ans);

    Json::ArrayIndex sz = res["keys"].size();
    if (sz > MAX_KEYS) sz = MAX_KEYS;
    for (Json::ArrayIndex i = 0; i < sz; ++i) {
        key.push_back(res["keys"][i].asString());
        is_lock.push_back(false);
    }

    sz = res["mode"].size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        std::string tmp = res["mode"][i].asString();
        modes.push_back(tmp);
        mode_prompt[tmp] = res[tmp];
        if (i == 0) {
            default_prompt = tmp;
        }
    }

    parse_json_to_set(res["black_list"], black_list);

    MAX_TOKEN = res["MAX_TOKEN"].asInt();
    MAX_REPLY = res["MAX_REPLY"].asInt();
    RED_LINE = res["RED_LINE"].asInt();
    base_url = res.get("base_url", "https://api.openai.com").asString();
    model_name = res.get("model", "gpt-3.5-turbo").asString();

    is_open = true;
    is_debug = false;
    key_cycle = 0;

    if (fs::exists(gpt_history_dir)) {
        fs::path gpt_files = gpt_history_dir;
        fs::directory_iterator di(gpt_files);
        std::regex history_file_regex(R"(^(-?\d+)\.json$)");
        for (auto &entry : di) {
            if (!entry.is_regular_file()) {
                continue;
            }
            std::string filename = entry.path().filename().string();
            std::smatch match;
            if (!std::regex_match(filename, match, history_file_regex)) {
                continue;
            }
            int64_t id = std::stoll(match[1].str());
            Json::Value J = string_to_json(readfile(entry.path()));
            history[id] = J["history"];
            pre_default[id] = J["pre_prompt"].asString();
        }
    }
}

void gpt3_5::save_file()
{
    std::lock_guard<std::recursive_mutex> lock(data_lock);
    Json::Value J;
    for (const std::string &u : key)
        J["keys"].append(u);
    for (const std::string &u : modes)
        J["mode"].append(u);
    J["black_list"] = parse_set_to_json(black_list);
    J["MAX_TOKEN"] = MAX_TOKEN;
    J["MAX_REPLY"] = MAX_REPLY;
    J["RED_LINE"] = RED_LINE;
    for (const std::string &u : modes) {
        J[u] = mode_prompt[u];
    }
    writefile(bot_config_path(nullptr, "features/openai/openai.json"),
              J.toStyledString());
}

int64_t getlength(const Json::Value &J)
{
    int64_t l = 0;
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        l += J[i]["content"].asString().length() / 0.75;
    }
    return l;
}

// https://stackoverflow.com/a/48212993/17792535
bool isASCII(const std::string &s)
{
    return !std::any_of(s.begin(), s.end(), [](char c) {
        return static_cast<unsigned char>(c) > 127;
    });
}

std::string gpt3_5::do_black(std::string message)
{
    std::lock_guard<std::recursive_mutex> lock(data_lock);
    bool filtered = false;
    for (std::string u : black_list) {
        if (u.empty()) continue;
        if (message.find(u) == std::string::npos) continue;

        filtered = true;
        // Escape basic regex special chars
        std::string escaped_u;
        for (char c : u) {
            if (strchr(".^$*+?()[]{}\\|", c)) escaped_u += '\\';
            escaped_u += c;
        }
        std::regex black_regex;
        try {
            if (isASCII(u))
                black_regex = std::regex("\\b" + escaped_u + "\\b");
            else
                black_regex = std::regex(escaped_u);
            message = std::regex_replace(message, black_regex, "__");
        } catch (...) {
            // If regex still fails, fallback to simple find/replace if needed
            size_t pos = 0;
            while ((pos = message.find(u, pos)) != std::string::npos) {
                message.replace(pos, u.length(), "__");
                pos += 2;
            }
        }
    }
    if (filtered) {
        message += "\n(QAQ 响应被过滤了)";
    }
    return message;
}

size_t gpt3_5::get_avaliable_key()
{
    std::lock_guard<std::recursive_mutex> lock(data_lock);
    size_t u;
    for (size_t i = 0; i < key.size(); i++) {
        if (!is_lock[u = (key_cycle + i) % key.size()]) {
            key_cycle = u;
            return u;
        }
    }
    return key_cycle;
}

void gpt3_5::save_history(int64_t id)
{
    std::lock_guard<std::recursive_mutex> lock(data_lock);
    Json::Value J;
    J["pre_prompt"] = pre_default[id];
    J["history"] = history[id];
    writefile(
        bot_config_path(nullptr, "gpt3_5/" + std::to_string(id) + ".json"),
        J.toStyledString());
}

void gpt3_5::fallback_trim_history(int64_t id, int rounds)
{
    std::lock_guard<std::recursive_mutex> lock(data_lock);
    Json::Value ign;
    for (int j = 0; j < rounds; ++j) {
        if (history[id].size() > 0) history[id].removeIndex(0, &ign);
        if (history[id].size() > 0) history[id].removeIndex(0, &ign);
        if (history[id].size() == 0) break;
    }
}

bool gpt3_5::compress_history(int64_t id, size_t keyid, const msg_meta &conf,
                              std::string *error_message)
{
    Json::Value old_history;
    std::string current_mode;
    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        if (history.find(id) == history.end() || history[id].size() <= 2) {
            if (error_message) *error_message = "history too short to compress.";
            return false;
        }
        old_history = history[id];
        current_mode = pre_default[id];
    }

    Json::ArrayIndex total_sz = old_history.size();
    Json::ArrayIndex recent_keep = static_cast<Json::ArrayIndex>(COMPRESS_RECENT_MESSAGES);
    if (total_sz <= recent_keep) {
        if (error_message) *error_message = "history too short to compress.";
        return false;
    }

    Json::ArrayIndex split_idx = total_sz - recent_keep;
    Json::Value older_history(Json::arrayValue);
    Json::Value recent_history(Json::arrayValue);
    for (Json::ArrayIndex i = 0; i < split_idx; ++i) {
        older_history.append(old_history[i]);
    }
    for (Json::ArrayIndex i = split_idx; i < total_sz; ++i) {
        recent_history.append(old_history[i]);
    }

    if (older_history.empty()) {
        if (error_message) *error_message = "history too short to compress.";
        return false;
    }

    perform_archive(id, conf, true, true);

    Json::Value req;
    req["model"] = model_name;
    req["temperature"] = 0.3;
    req["max_tokens"] = MAX_REPLY;

    Json::Value messages(Json::arrayValue);
    Json::Value prompt_messages = mode_prompt[current_mode.empty() ? default_prompt : current_mode];
    for (Json::ArrayIndex i = 0; i < prompt_messages.size(); ++i) {
        messages.append(prompt_messages[i]);
    }
    for (Json::ArrayIndex i = 0; i < older_history.size(); ++i) {
        messages.append(older_history[i]);
    }

    Json::Value user_msg;
    user_msg["role"] = "user";
    user_msg["content"] =
        "Provide a detailed summary of the previous conversation for continuing in a new session.\n\n"
        "The new session will not have access to the original conversation history, so preserve all context needed to continue seamlessly.\n\n"
        "Focus on:\n"
        "- Key topics discussed and why they matter\n"
        "- Important decisions made and their reasoning\n"
        "- Current work in progress and its state\n"
        "- Next steps or open questions to address\n"
        "- The participants in the conversation, especially recurring group members, and their speaking style, personality, preferences, habits, and relationship dynamics\n"
        "- Stable details that help imitate the established tone when continuing to talk with those same people later\n"
        "- Any relevant technical details, code snippets, or configurations mentioned\n\n"
        "Requirements:\n"
        "1. Write in aforementioned language, matching the original conversation language\n"
        "2. Be concise but complete — do not omit important context\n"
        "3. Output the summary directly without prefaces or meta-commentary\n"
        "4. Start with a clear indicator (e.g., \"[Summary of previous conversation]\" or equivalent)\n"
        "5. Preserve user-by-user memory when possible: if different people have distinct tones or repeated viewpoints, describe them separately so the assistant can continue interacting naturally after compression\n\n"
        "Summarize the conversation above directly.";
    messages.append(user_msg);
    req["messages"] = messages;

    Json::Value resp;
    try {
        resp = string_to_json(do_post(base_url, "/v1/chat/completions", false,
                                      req,
                                      {{"Content-Type", "application/json"},
                                       {"Authorization", "Bearer " + key[keyid]}},
                                      true));
    }
    catch (std::string e) {
        if (error_message) *error_message = e;
        return false;
    }
    catch (...) {
        if (error_message) *error_message = "http connection failed.";
        return false;
    }

    if (resp.isMember("error")) {
        if (error_message) *error_message = resp["error"]["message"].asString();
        return false;
    }
    if (!resp.isMember("choices") || !resp["choices"].isArray() ||
        resp["choices"].empty()) {
        if (error_message) *error_message = "summary response format invalid";
        return false;
    }

    std::string summary = trim(resp["choices"][0]["message"]["content"].asString());
    if (summary.empty()) {
        if (error_message) *error_message = "summary is empty";
        return false;
    }

    Json::Value new_history(Json::arrayValue);
    Json::Value summary_msg;
    summary_msg["role"] = "system";
    summary_msg["content"] = summary;
    new_history.append(summary_msg);
    for (Json::ArrayIndex i = 0; i < recent_history.size(); ++i) {
        new_history.append(recent_history[i]);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        history[id] = new_history;
        if (pre_default.find(id) == pre_default.end()) {
            pre_default[id] = current_mode.empty() ? default_prompt : current_mode;
        }
    }
    return true;
}


std::string gpt3_5::get_quoted_content(const bot *p, int64_t reply_id, int depth)
{
    if (depth > 5) return "...(too deep)";

    Json::Value get_msg_param;
    get_msg_param["message_id"] = reply_id;
    Json::Value msg_info =
        string_to_json(p->cq_send("get_msg", get_msg_param));

    if (msg_info["retcode"].asInt() != 0 || !msg_info.isMember("data")) return "[Failed to fetch message]";

    Json::Value &msg_data = msg_info["data"];
    if (!msg_data.isMember("message")) return "[Empty message content]";

    std::string content = messageArr_to_string(msg_data["message"]);

    // Check for forward message with ID
    size_t fwd_pos = content.find("[CQ:forward,id=");
    if (fwd_pos != std::string::npos) {
        size_t id_start = fwd_pos + 15;
        size_t id_end = content.find_first_of(",]", id_start);
        if (id_end != std::string::npos) {
            std::string forward_id = content.substr(id_start, id_end - id_start);
            return expand_forward_content(p, forward_id, depth);
        }
    }

    // Check for forward message with raw content (JSON format)
    size_t fwd_cont_pos = content.find("[CQ:forward,content=");
    if (fwd_cont_pos != std::string::npos) {
        size_t cont_start = fwd_cont_pos + 20;
        size_t cont_end = content.find_last_of(']'); // Find the outermost CQ code closure
        if (cont_end != std::string::npos && cont_end > cont_start) {
            std::string raw_json = content.substr(cont_start, cont_end - cont_start);
            // Decode potential HTML entities (like &#91; for [)
            raw_json = std::regex_replace(raw_json, std::regex("&#91;"), "[");
            raw_json = std::regex_replace(raw_json, std::regex("&#93;"), "]");
            raw_json = std::regex_replace(raw_json, std::regex("&#44;"), ",");
            
            try {
                Json::Value fwd_data = string_to_json(raw_json);
                if (fwd_data.isArray()) {
                    Json::Value mock_fwd;
                    mock_fwd["retcode"] = 0;
                    mock_fwd["data"]["messages"] = fwd_data;
                    
                    // We need a way to reuse the formatting logic in expand_forward_content
                    // Since expand_forward_content currently fetches, we'll manually format here or refactor.
                    // For now, let's process it directly to be safe.
                    std::string result = (depth == 0) ? "合并转发记录:" : "";
                    static const std::regex cq_regex(R"(\[CQ:([^,\]]+)[^\]]*\])");
                    for (const auto &m : fwd_data) {
                        std::string msg_text = messageArr_to_string(m.isMember("message") ? m["message"] : m["content"]);
                        std::replace(msg_text.begin(), msg_text.end(), '\n', ' ');
                        std::replace(msg_text.begin(), msg_text.end(), '\r', ' ');
                        
                        std::string cleaned;
                        auto words_begin = std::sregex_iterator(msg_text.begin(), msg_text.end(), cq_regex);
                        auto words_end = std::sregex_iterator();
                        size_t last_pos = 0;
                        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                            std::smatch match = *i;
                            cleaned += msg_text.substr(last_pos, match.position() - last_pos);
                            std::string type = match[1].str();
                            if (type == "image") cleaned += "[图片]";
                            else if (type == "record") cleaned += "[语音]";
                            else if (type == "face") cleaned += "[表情]";
                            else if (type == "at") cleaned += "[@某人]";
                            else cleaned += "[" + type + "]";
                            last_pos = match.position() + match.length();
                        }
                        cleaned += msg_text.substr(last_pos);
                        
                        std::string nick = m["sender"]["nickname"].asString();
                        uint64_t uid = m["sender"]["user_id"].asUInt64();
                        if (uid != 0) result += "[" + nick + "(" + std::to_string(uid) + ")]：" + cleaned;
                        else result += "[" + nick + "]：" + cleaned;
                    }
                    return trim(result);
                }
            } catch (...) {}
        }
    }

    std::string nickname = "Unknown";
    if (msg_data.isMember("sender") && msg_data["sender"].isMember("nickname") && msg_data["sender"]["nickname"].isString()) {
        nickname = msg_data["sender"]["nickname"].asString();
    }

    uint64_t uid = 0;
    if (msg_data.isMember("sender") && msg_data["sender"].isMember("user_id")) {
        uid = msg_data["sender"]["user_id"].asUInt64();
    }

    if (uid != 0) {
        return "[" + nickname + "(" + std::to_string(uid) + ")]：" + content;
    }
    return "[" + nickname + "]：" + content;
}

std::string gpt3_5::expand_forward_content(const bot *p, const std::string &forward_id, int depth)
{
    Json::Value get_fwd_param;
    get_fwd_param["id"] = forward_id;
    Json::Value fwd_info = string_to_json(
        p->cq_send("get_forward_msg", get_fwd_param));

    if (fwd_info["retcode"].asInt() != 0 || !fwd_info.isMember("data")) return "[Failed to fetch forward message]";

    Json::Value messages = fwd_info["data"].isMember("messages")
                               ? fwd_info["data"]["messages"]
                               : fwd_info["data"];
    std::string result = (depth == 0) ? "合并转发记录:" : "";
    if (messages.isArray()) {
        static const std::regex cq_regex(R"(\[CQ:([^,\]]+)[^\]]*\])");
        for (const auto &m : messages) {
            if (!m.isMember("message") && !m.isMember("content")) continue;

            std::string content = messageArr_to_string(
                m.isMember("message") ? m["message"] : m["content"]);

            // Check for nested forward
            size_t fwd_pos = content.find("[CQ:forward,id=");
            if (fwd_pos != std::string::npos) {
                size_t id_start = fwd_pos + 15;
                size_t id_end = content.find(']', id_start);
                if (id_end != std::string::npos) {
                    std::string nested_id =
                        content.substr(id_start, id_end - id_start);
                    result += "\n" + expand_forward_content(p, nested_id, depth + 1);
                    continue;
                }
            }

            // Flatten newlines
            std::replace(content.begin(), content.end(), '\n', ' ');
            std::replace(content.begin(), content.end(), '\r', ' ');

            // Simplify CQ codes
            std::string cleaned_content;
            auto words_begin = std::sregex_iterator(content.begin(), content.end(), cq_regex);
            auto words_end = std::sregex_iterator();
            size_t last_pos = 0;
            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                cleaned_content += content.substr(last_pos, match.position() - last_pos);
                std::string type = match[1].str();
                if (type == "image") cleaned_content += "[图片]";
                else if (type == "record") cleaned_content += "[语音]";
                else if (type == "face") cleaned_content += "[表情]";
                else if (type == "video") cleaned_content += "[视频]";
                else if (type == "at") cleaned_content += "[@某人]";
                else if (type == "reply") cleaned_content += "[回复]";
                else cleaned_content += "[" + type + "]";
                last_pos = match.position() + match.length();
            }
            cleaned_content += content.substr(last_pos);
            content = cleaned_content;

            if (!m.isMember("sender")) continue;

            std::string nick = m["sender"]["nickname"].isString() 
                               ? m["sender"]["nickname"].asString() 
                               : "Unknown";
            uint64_t uid = m["sender"]["user_id"].asUInt64();

            if (uid != 0) {
                result += "[" + nick + "(" + std::to_string(uid) + ")]：" + content;
            } else {
                result += "[" + nick + "]：" + content;
            }
        }
    }
    return trim(result);
}

void gpt3_5::process(std::string message, const msg_meta &conf)
{
    int64_t reply_id = -1;
    if (starts_with(message, "[CQ:reply,id=")) {
        size_t id_start = 13;
        size_t id_end = message.find_first_of(",]", id_start);
        if (id_end != std::string::npos) {
            reply_id = std::stoll(message.substr(id_start, id_end - id_start));
        }
        size_t pos = message.find(']');
        if (pos != std::string::npos) {
            message = trim(message.substr(pos + 1));
        }
    }

    if (cmd_match_exact(message, {"你说的话我不喜欢"}) && reply_id != -1) {
        Json::Value get_msg_param;
        get_msg_param["message_id"] = reply_id;
        Json::Value msg_info =
            string_to_json(conf.p->cq_send("get_msg", get_msg_param));
        if (msg_info["data"]["sender"]["user_id"].asUInt64() ==
            conf.p->get_botqq()) {
            Json::Value del_msg_param;
            del_msg_param["message_id"] = reply_id;
            conf.p->cq_send("delete_msg", del_msg_param);
        }
        return;
    }

    if (!starts_with(message, ".ai")) {
        return;
    }

    message = do_black(trim(message.substr(3)));

    std::istringstream iss(message);
    std::string command;
    iss >> command;
    std::string args;
    getline(iss, args);
    args = trim(args);

    int64_t id = conf.message_type == "group" ? (conf.group_id << 1)
                                              : ((conf.user_id << 1) | 1);

    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        if (history.find(id) == history.end()) {
            pre_default[id] = default_prompt;
        }
    }

    // Auto-archive check
    perform_archive(id, conf, true);


    const std::vector<cmd_exact_rule> exact_rules = {
        {".test",
         [&]() {
             conf.p->cq_send(message, conf);
             return true;
         }},
        {".arc",
         [&]() {
             if (!is_allowed_arc(id, conf)) {
                 conf.p->cq_send("Not on op list.", conf);
                 return true;
             }
             std::istringstream arc_iss(args);
             std::string sub_cmd;
             arc_iss >> sub_cmd;
             if (sub_cmd == "list") {
                 int page = 1;
                 arc_iss >> page;
                 list_archives(id, conf, page);
             }
             else if (sub_cmd == "restore") {
                 std::string target;
                 arc_iss >> target;
                 if (target.empty()) {
                     conf.p->cq_send("用法: .ai arc restore [编号/文件名]",
                                     conf);
                 }
                 else {
                     restore_archive(id, conf, target);
                 }
             }
             else {
                 perform_archive(id, conf, false);
             }
             return true;
         }},
        {"arc",
         [&]() {
             if (!is_allowed_arc(id, conf)) {
                 conf.p->cq_send("Not on op list.", conf);
                 return true;
             }
             std::istringstream arc_iss(args);
             std::string sub_cmd;
             arc_iss >> sub_cmd;
             if (sub_cmd == "list") {
                 int page = 1;
                 arc_iss >> page;
                 list_archives(id, conf, page);
             }
             else if (sub_cmd == "restore") {
                 std::string target;
                 arc_iss >> target;
                 if (target.empty()) {
                     conf.p->cq_send("用法: .ai arc restore [编号/文件名]",
                                     conf);
                 }
                 else {
                     restore_archive(id, conf, target);
                 }
             }
             else {
                 perform_archive(id, conf, false);
             }
             return true;
         }},
        {".reset",
        [&]() {
            {
                std::lock_guard<std::recursive_mutex> lock(data_lock);
                history[id].clear();
            }
            save_history(id);
            conf.p->cq_send("reset done.", conf);
            return true;
        }},
        {"reset",
        [&]() {
            {
                std::lock_guard<std::recursive_mutex> lock(data_lock);
                history[id].clear();
            }
            save_history(id);
            conf.p->cq_send("reset done.", conf);
            return true;
        }},
        {".compress",
         [&]() {
             if (key.size() == 0) {
                 conf.p->cq_send("No avaliable key!", conf);
                 return true;
             }
             size_t compress_keyid = get_avaliable_key();
             {
                 std::lock_guard<std::recursive_mutex> lock(data_lock);
                 if (is_lock[compress_keyid]) {
                     conf.p->cq_send("请等待其他对话中输入的回复。", conf);
                     return true;
                 }
                 if (active_ids.count(id)) {
                     conf.p->cq_send("请等待该对话中上一个输入的回复。", conf);
                     return true;
                 }
                 if (!is_open) {
                     conf.p->cq_send("已关闭。" + close_message, conf);
                     return true;
                 }
                 if (history.find(id) == history.end()) {
                     pre_default[id] = default_prompt;
                 }
                 is_lock[compress_keyid] = true;
                 active_ids.insert(id);
             }

             std::lock_guard<std::mutex> compress_lock(gptlock[compress_keyid]);
             std::string compress_error;
             bool compressed = compress_history(id, compress_keyid, conf, &compress_error);
             {
                 std::lock_guard<std::recursive_mutex> lock(data_lock);
                 is_lock[compress_keyid] = false;
                 active_ids.erase(id);
             }
             if (compressed) {
                 save_history(id);
                 conf.p->cq_send("compress done.", conf);
             }
             else {
                 conf.p->cq_send(compress_error.empty() ? "compress failed." : compress_error,
                                 conf);
             }
             return true;
         }},
        {"compress",
         [&]() {
             if (key.size() == 0) {
                 conf.p->cq_send("No avaliable key!", conf);
                 return true;
             }
             size_t compress_keyid = get_avaliable_key();
             {
                 std::lock_guard<std::recursive_mutex> lock(data_lock);
                 if (is_lock[compress_keyid]) {
                     conf.p->cq_send("请等待其他对话中输入的回复。", conf);
                     return true;
                 }
                 if (active_ids.count(id)) {
                     conf.p->cq_send("请等待该对话中上一个输入的回复。", conf);
                     return true;
                 }
                 if (!is_open) {
                     conf.p->cq_send("已关闭。" + close_message, conf);
                     return true;
                 }
                 if (history.find(id) == history.end()) {
                     pre_default[id] = default_prompt;
                 }
                 is_lock[compress_keyid] = true;
                 active_ids.insert(id);
             }

             std::lock_guard<std::mutex> compress_lock(gptlock[compress_keyid]);
             std::string compress_error;
             bool compressed = compress_history(id, compress_keyid, conf, &compress_error);
             {
                 std::lock_guard<std::recursive_mutex> lock(data_lock);
                 is_lock[compress_keyid] = false;
                 active_ids.erase(id);
             }
             if (compressed) {
                 save_history(id);
                 conf.p->cq_send("compress done.", conf);
             }
             else {
                 conf.p->cq_send(compress_error.empty() ? "compress failed." : compress_error,
                                 conf);
             }
             return true;
         }},
        {".change",
        [&]() {
            std::string reply;
            bool need_save = false;
            {
                std::lock_guard<std::recursive_mutex> lock(data_lock);
                if (conf.p->is_op(conf.user_id) || (id & 1)) {
                    const std::string mode = args;
                    bool flg = false;
                    reply = "avaliable modes:";
                    for (const std::string &u : modes) {
                        reply += " " + u;
                        if (u == mode) {
                            flg = true;
                            history[id].clear();
                            pre_default[id] = mode;
                            reply = "change done.";
                            need_save = true;
                            break;
                        }
                    }
                }
                else {
                    reply = "Not on op list.";
                }
            }
            conf.p->cq_send(reply, conf);
            if (need_save) {
                save_history(id);
            }
            return true;
        }},
        {".sw",
        [&]() {
            bool new_state;
            {
                std::lock_guard<std::recursive_mutex> lock(data_lock);
                if (conf.p->is_op(conf.user_id)) {
                    is_open = !is_open;
                    close_message = args;
                    new_state = is_open;
                } else {
                    conf.p->cq_send("Not on op list.", conf);
                    return true;
                }
            }
            conf.p->cq_send("is_open: " + std::to_string(new_state), conf);
            return true;
        }},
        {".debug",
        [&]() {
            bool new_state;
            {
                std::lock_guard<std::recursive_mutex> lock(data_lock);
                if (conf.p->is_op(conf.user_id)) {
                    is_debug = !is_debug;
                    new_state = is_debug;
                } else {
                    conf.p->cq_send("Not on op list.", conf);
                    return true;
                }
            }
            conf.p->cq_send("is_debug: " + std::to_string(new_state), conf);
            return true;
        }},
        {".set",
        [&]() {
            std::string reply = "Not on op list.";
            bool do_save = false;
            {
                std::lock_guard<std::recursive_mutex> lock(data_lock);
                if (conf.p->is_op(conf.user_id)) {
                    std::string type;
                    int64_t num = 0;
                    std::istringstream arg_iss(args);
                    if (!(arg_iss >> type >> num)) {
                        reply = "Unknown type";
                    } else if (type == "reply") {
                        MAX_REPLY = num;
                        reply = "set MAX_REPLY to " + std::to_string(num);
                        do_save = true;
                    } else if (type == "token") {
                        MAX_TOKEN = num;
                        reply = "set MAX_TOKEN to " + std::to_string(num);
                        do_save = true;
                    } else if (type == "red") {
                        RED_LINE = num;
                        reply = "set RED_LINE to " + std::to_string(num);
                        do_save = true;
                    } else {
                        reply = "Unknown type";
                    }
                }
            }
            conf.p->cq_send(reply, conf);
            if (do_save) save_file();
            return true;
        }},
    };

    bool handled = false;
    (void)cmd_try_dispatch(command, exact_rules, {}, handled);
    if (handled) {
        return;
    }

    if (key.size() == 0) {
        conf.p->cq_send("No avaliable key!", conf);
        return;
    }

    size_t keyid = get_avaliable_key();
    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        if (is_lock[keyid]) {
            conf.p->cq_send("请等待其他对话中输入的回复。", conf);
            return;
        }
        if (active_ids.count(id)) {
            conf.p->cq_send("请等待该对话中上一个输入的回复。", conf);
            return;
        }
        if (!is_open) {
            conf.p->cq_send("已关闭。" + close_message, conf);
            return;
        }
        is_lock[keyid] = true;
        active_ids.insert(id);
    }

    std::lock_guard<std::mutex> lock(gptlock[keyid]);
    Json::Value J, user_input_J, ign;
    user_input_J["role"] = "user";
    std::string nickname = get_stranger_name(conf.p, conf.user_id);

    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    now += 8 * 3600;
    tm tm_utc8_res = *std::gmtime(&now);
    char time_buf[64] = "Unknown time";
    std::strftime(time_buf, sizeof(time_buf), "%Y/%m/%d %H:%M:%S", &tm_utc8_res);

    std::string prompt_content = "[User: " + std::to_string(conf.user_id) + " (" +
                                 nickname + ")] [Time: " + std::string(time_buf) + "]";
    if (reply_id != -1) {
        prompt_content += " [CQ:reply,id=" + std::to_string(reply_id) + "] 引用聊天记录：" + get_quoted_content(conf.p, reply_id);
    }
    prompt_content += " 正文：" + message;
    user_input_J["content"] = prompt_content;

    J["model"] = model_name;
    
    {
        std::unique_lock<std::recursive_mutex> lock_data(data_lock);

        // Prefer compression before fallback trimming.
        Json::Value &h = history[id];
        while (getlength(h) > (int64_t)(MAX_TOKEN - RED_LINE) && h.size() > 2) {
            lock_data.unlock();
            std::string compress_error;
            bool compressed = compress_history(id, keyid, conf, &compress_error);
            lock_data.lock();
            if (!compressed) {
                break;
            }
        }
        while (getlength(h) > (int64_t)(MAX_TOKEN - MAX_REPLY)) {
            if (h.size() <= 2) break;
            if (h.size() > 0) h.removeIndex(0, &ign);
            if (h.size() > 0) h.removeIndex(0, &ign);
        }

        Json::Value K = mode_prompt[pre_default[id]];
        Json::ArrayIndex sz = h.size();
        for (Json::ArrayIndex i = 0; i < sz; i++) {
            K.append(h[i]);
        }
        K.append(user_input_J);
        J["messages"] = K;
    }
    
    J["temperature"] = 0.7;
    J["max_tokens"] = MAX_REPLY;
    try {
        J = string_to_json(do_post(base_url, "/v1/chat/completions", false, J,
                                   {{"Content-Type", "application/json"},
                                    {"Authorization", "Bearer " + key[keyid]}},
                                   true));
    }
    catch (std::string e) {
        J.clear();
        J["error"]["message"] = e;
    }
    catch (...) {
        J.clear();
        J["error"]["message"] = "http connection failed.";
    }
    conf.p->setlog(LOG::INFO, "openai: user " + std::to_string(conf.user_id));
    
    {
        std::lock_guard<std::recursive_mutex> lock_data(data_lock);
        is_lock[keyid] = false;
        active_ids.erase(id);
    }

    if (J.isMember("error")) {
        if (J["error"]["message"].asString().find(
                "This model's maximum context length is") == 0) {
            conf.p->cq_send("Openai ERROR: history message is too long. Please "
                            "try again or try .ai.reset",
                            conf);
            {
                std::lock_guard<std::recursive_mutex> lock_data(data_lock);
                if (history[id].size() > 0) history[id].removeIndex(0, &ign);
                if (history[id].size() > 0) history[id].removeIndex(0, &ign);
            }
            save_history(id);
        }
        else {
            conf.p->cq_send("Openai ERROR: " + J["error"]["message"].asString(),
                            conf);
        }
    }
    else {
        if (!J.isMember("choices") || !J["choices"].isArray() ||
            J["choices"].empty()) {
            conf.p->cq_send("Openai ERROR: API 响应格式异常(缺少 choices)",
                            conf);
            {
                std::lock_guard<std::recursive_mutex> lock_data(data_lock);
                active_ids.erase(id);
            }
            return;
        }

        std::string aimsg = J["choices"][0]["message"]["content"].asString();
        std::string finish_reason = J["choices"][0].isMember("finish_reason") 
                                    ? J["choices"][0]["finish_reason"].asString() 
                                    : "";

        if (trim(aimsg).empty()) {
            if (finish_reason == "content_filter") {
                conf.p->cq_send("QAQ 响应被 OpenAI 安全策略过滤了", conf);
            } else {
                conf.p->cq_send("API空返回！", conf);
            }
            {
                std::lock_guard<std::recursive_mutex> lock_data(data_lock);
                active_ids.erase(id);
            }
            return;
        }

        aimsg = do_black(aimsg);

        if (finish_reason == "length") {
            aimsg += "\n(QAQ 响应因长度限制未完成)";
        } else if (finish_reason == "content_filter") {
            aimsg += "\n(QAQ 响应被 OpenAI 安全策略部分过滤)";
        }

        int tokens = 0;
        if (J.isMember("usage") && J["usage"].isMember("total_tokens")) {
            tokens = static_cast<int>(J["usage"]["total_tokens"].asInt64());
        }

        std::string reply_msg = "[CQ:reply,id=" + std::to_string(conf.message_id) +
                                "] " + aimsg;
    {
        std::unique_lock<std::recursive_mutex> lock_data(data_lock);
            if (MAX_TOKEN < tokens) {
                history[id].clear();
            }
            else {
                if (tokens > MAX_TOKEN - RED_LINE) {
                    lock_data.unlock();
                    std::string compress_error;
                    bool compressed = compress_history(id, keyid, conf, &compress_error);
                    lock_data.lock();
                    if (!compressed) {
                        for (int i = 5; i >= 1; i--) {
                            if (MAX_TOKEN - RED_LINE / i < tokens) {
                                lock_data.unlock();
                                fallback_trim_history(id, i);
                                lock_data.lock();
                                break;
                            }
                        }
                    }
                }
            }
            J.clear();
            J["role"] = "assistant";
            J["content"] = aimsg;
            history[id].append(user_input_J);
            history[id].append(J);
        }
        conf.p->cq_send(reply_msg, conf);
    }
    save_history(id);
}

bool gpt3_5::check(std::string message, const msg_meta &conf)
{
    (void)conf;
    if (starts_with(message, "[CQ:reply,id=")) {
        size_t pos = message.find(']');
        if (pos != std::string::npos) {
            message = trim(message.substr(pos + 1));
        }
    }
    if (cmd_match_exact(message, {"你说的话我不喜欢"})) {
        return true;
    }
    return cmd_match_prefix(message, {".ai"});
}

std::string gpt3_5::help()
{
    return "OpenAI GPT-3.5：使用 .ai [内容] 开始对话\n"
           "指令列表：\n"
           ".ai reset - 重置当前对话上下文\n"
           ".ai compress / .ai.compress - 压缩旧上下文并保留最近对话\n"
           ".ai change [模式] - 切换提示词模式\n"
           ".ai arc - 手动归档当前上下文\n"
           ".ai arc list [页码] - 查看归档列表（每页5条）\n"
           ".ai arc restore [编号/文件名] - 从归档中恢复上下文\n"
           "权限说明：归档与恢复功能在群聊中需管理员（OP）权限，私聊可直接使用。";
}

uintmax_t gpt3_5::get_archives_total_size()
{
    std::string backup_root = bot_config_path(nullptr, "gpt3_5/backups");
    if (!fs::exists(backup_root)) return 0;
    uintmax_t total_size = 0;
    for (const auto &entry : fs::recursive_directory_iterator(backup_root)) {
        if (entry.is_regular_file()) {
            total_size += entry.file_size();
        }
    }
    return total_size;
}

void gpt3_5::perform_archive(int64_t id, const msg_meta &conf, bool is_auto,
                             bool silent)
{
    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        if (arc_is_full) {
            if (!is_auto) {
                conf.p->cq_send("当前归档文件过大，已暂停生成。请联系管理员", conf);
            }
            return;
        }
    }

    std::string backup_dir =
        bot_config_path(nullptr, "gpt3_5/backups/" + std::to_string(id));
    if (!fs::exists(backup_dir)) {
        fs::create_directories(backup_dir);
    }

    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    now += 8 * 3600;
    tm tm_res = *std::gmtime(&now);
    char time_buf[64];
    if (is_auto) {
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d_auto", &tm_res);
    } else {
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d_%H-%M-%S", &tm_res);
    }

    std::string filename = std::string(time_buf) + ".json";
    std::string full_path = backup_dir + "/" + filename;

    if (is_auto && fs::exists(full_path)) return;

    Json::Value J;
    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        J["pre_prompt"] = pre_default[id];
        J["history"] = history[id];
        writefile(full_path, J.toStyledString());
    }

    bool need_check = false;
    {
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        if (++arc_check_counter >= 10) {
            arc_check_counter = 0;
            need_check = true;
        }
    }
    if (need_check) {
        bool full = (get_archives_total_size() >= 250 * 1024 * 1024);
        std::lock_guard<std::recursive_mutex> lock(data_lock);
        arc_is_full = full;
    }

    if (!is_auto && !silent) {
        conf.p->cq_send("归档已生成: " + filename, conf);
    }
}

bool gpt3_5::is_allowed_arc(int64_t id, const msg_meta &conf)
{
    if (id & 1) return true; // Private chat
    return conf.p->is_op(conf.user_id);
}

void gpt3_5::list_archives(int64_t id, const msg_meta &conf, int page)
{
    std::string backup_dir =
        bot_config_path(nullptr, "gpt3_5/backups/" + std::to_string(id));
    if (!fs::exists(backup_dir)) {
        conf.p->cq_send("暂无归档记录。", conf);
        return;
    }

    std::vector<std::string> files;
    for (const auto &entry : fs::directory_iterator(backup_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path().filename().string());
        }
    }
    std::sort(files.rbegin(), files.rend()); // Newest first

    if (files.empty()) {
        conf.p->cq_send("暂无归档记录。", conf);
        return;
    }

    int total_pages = (files.size() + 4) / 5;
    if (page < 1) page = 1;
    if (page > total_pages) page = total_pages;

    std::string res = "归档列表 (第 " + std::to_string(page) + "/" +
                      std::to_string(total_pages) + " 页):\n";
    for (size_t i = (page - 1) * 5; i < page * 5 && i < files.size(); ++i) {
        res += std::to_string(i + 1) + ". " + files[i] + "\n";
    }
    res += "使用 .ai arc restore [编号/文件名] 恢复。";
    conf.p->cq_send(trim(res), conf);
}

void gpt3_5::restore_archive(int64_t id, const msg_meta &conf,
                             const std::string &arg)
{
    std::string backup_dir =
        bot_config_path(nullptr, "gpt3_5/backups/" + std::to_string(id));
    if (!fs::exists(backup_dir)) {
        conf.p->cq_send("暂无归档记录。", conf);
        return;
    }

    std::string target_file;
    if (std::all_of(arg.begin(), arg.end(), ::isdigit)) {
        int idx = std::stoi(arg) - 1;
        std::vector<std::string> files;
        for (const auto &entry : fs::directory_iterator(backup_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path().filename().string());
            }
        }
        std::sort(files.rbegin(), files.rend());
        if (idx >= 0 && idx < (int)files.size()) {
            target_file = files[idx];
        }
    }
    else {
        target_file = arg;
        if (target_file.find(".json") == std::string::npos)
            target_file += ".json";
    }

    if (target_file.empty()) return;

    if (target_file.find("..") != std::string::npos || 
        target_file.find('/') != std::string::npos || 
        target_file.find('\\') != std::string::npos) {
        conf.p->cq_send("非法的文件名格式。", conf);
        return;
    }

    std::string full_path = backup_dir + "/" + target_file;
    if (!fs::exists(full_path)) {
        conf.p->cq_send("找不到指定的归档文件: " + target_file, conf);
        return;
    }

    Json::Value J = string_to_json(readfile(full_path));
    if (J.isMember("history") && J.isMember("pre_prompt")) {
        {
            std::lock_guard<std::recursive_mutex> lock(data_lock);
            history[id] = J["history"];
            pre_default[id] = J["pre_prompt"].asString();
        }
        save_history(id);
        conf.p->cq_send("归档 " + target_file + " 已成功恢复。", conf);
    }
    else {
        conf.p->cq_send("归档文件格式错误。", conf);
    }
}

DECLARE_FACTORY_FUNCTIONS(gpt3_5)
