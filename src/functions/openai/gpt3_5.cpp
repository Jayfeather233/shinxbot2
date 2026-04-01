#include "gpt3_5.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>

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
    else {
        std::string ans = readfile(openai_conf_path);

        Json::Value res = string_to_json(ans);

        Json::ArrayIndex sz = res["keys"].size();
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
    }
    is_open = true;
    is_debug = false;
    key_cycle = 0;

    if (fs::exists(gpt_history_dir)) {
        fs::path gpt_files = gpt_history_dir;
        fs::directory_iterator di(gpt_files);
        for (auto &entry : di) {
            if (entry.is_regular_file()) {
                std::istringstream iss(entry.path().filename().string());
                int64_t id;
                iss >> id;
                Json::Value J = string_to_json(readfile(entry.path()));
                history[id] = J["history"];
                pre_default[id] = J["pre_prompt"].asString();
            }
        }
    }
}

void gpt3_5::save_file()
{
    Json::Value J;
    for (std::string u : key)
        J["keys"].append(u);
    for (std::string u : modes)
        J["mode"].append(u);
    J["black_list"] = parse_set_to_json(black_list);
    J["MAX_TOKEN"] = MAX_TOKEN;
    J["MAX_REPLY"] = MAX_REPLY;
    J["RED_LINE"] = RED_LINE;
    for (std::string u : modes) {
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
    for (std::string u : black_list) {
        std::regex black_regex;
        if (isASCII(u))
            black_regex = std::regex("\\b" + u + "\\b");
        else
            black_regex = std::regex(u);
        message = std::regex_replace(message, black_regex, "__");
    }
    return message;
}

size_t gpt3_5::get_avaliable_key()
{
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
    Json::Value J;
    J["pre_prompt"] = pre_default[id];
    J["history"] = history[id];
    writefile(
        bot_config_path(nullptr, "gpt3_5/" + std::to_string(id) + ".json"),
        J.toStyledString());
}

void gpt3_5::process(std::string message, const msg_meta &conf)
{
<<<<<<< HEAD
    // Handle recall: if user replies with "你说的话我不喜欢", the bot recalls the replied-to message if it was sent by the bot.
    if (message.find("你说的话我不喜欢") != std::string::npos &&
        message.find("[CQ:reply,id=") != std::string::npos) {
        size_t pos = message.find("[CQ:reply,id=");
        size_t end_pos = message.find("]", pos);
        if (pos != std::string::npos && end_pos != std::string::npos) {
            std::string reply_id_str =
                message.substr(pos + 13, end_pos - (pos + 13));
            int64_t reply_id = my_string2int64(reply_id_str);

            Json::Value J;
            J["message_id"] = reply_id;
            std::string res = conf.p->cq_send("get_msg", J);
            Json::Value res_json = string_to_json(res);

            if (res_json.isMember("data") &&
                res_json["data"].isMember("sender") &&
                res_json["data"]["sender"]["user_id"].asUInt64() ==
                    conf.p->get_botqq()) {
                Json::Value del_J;
                del_J["message_id"] = reply_id;
                conf.p->cq_send("delete_msg", del_J);
                conf.p->setlog(LOG::INFO,
                               fmt::format("gpt3_5: Recalled message {} as "
                                           "requested by user {}",
                                           reply_id, conf.user_id));
                return;
            }
        }
    }

    // Handle .ai command: extract the content after .ai
    size_t ai_pos = message.find(".ai");
    if (ai_pos == std::string::npos) {
        return;
    }

    std::string command = trim(message.substr(ai_pos + 3));

    // Handle forwarded message reading: if command starts with .fw, read the replied-to forwarded message content.
    if (command.find(".fw") == 0) {
        size_t reply_pos = message.find("[CQ:reply,id=");
        if (reply_pos != std::string::npos) {
            size_t end_pos = message.find("]", reply_pos);
            std::string reply_id_str =
                message.substr(reply_pos + 13, end_pos - (reply_pos + 13));
            int64_t reply_id = my_string2int64(reply_id_str);

            Json::Value J;
            J["message_id"] = reply_id;
            std::string res = conf.p->cq_send("get_forward_msg", J);
            Json::Value res_json = string_to_json(res);

            if (res_json["retcode"].asInt() == 0) {
                std::string forward_content = "";
                for (const auto &node : res_json["data"]["messages"]) {
                    forward_content +=
                        "[" + node["sender"]["nickname"].asString() + "]: " +
                        messageArr_to_string(node["message"]) + "\n";
                }
                command = trim(command.substr(3)) +
                          "\n以下是聊天记录：\n" + forward_content;
            }
            else {
                conf.p->cq_send("获取转发消息失败", conf);
                return;
            }
        }
        else {
            conf.p->cq_send("请回复一个合并转发消息并使用 .ai .fw [你的要求]",
                            conf);
            return;
        }
    }

    message = do_black(trim(command));
    if (message.find(".test") == 0) {
        conf.p->cq_send(message, conf);
        return;
    }
=======
    message = do_black(trim(message.substr(3)));
    std::istringstream iss(message);
    std::string command;
    iss >> command;
    std::string args;
    getline(iss, args);
    args = trim(args);

>>>>>>> 940e730c8833c0a4b2562e8db11f14350afb6c8b
    int64_t id = conf.message_type == "group" ? (conf.group_id << 1)
                                              : ((conf.user_id << 1) | 1);

    const std::vector<cmd_exact_rule> exact_rules = {
        {".test",
         [&]() {
             conf.p->cq_send(message, conf);
             return true;
         }},
        {".reset",
         [&]() {
             auto it = history.find(id);
             if (it != history.end()) {
                 it->second.clear();
             }
             conf.p->cq_send("reset done.", conf);
             save_history(id);
             return true;
         }},
        {"reset",
         [&]() {
             auto it = history.find(id);
             if (it != history.end()) {
                 it->second.clear();
             }
             conf.p->cq_send("reset done.", conf);
             save_history(id);
             return true;
         }},
        {".change",
         [&]() {
             if (conf.p->is_op(conf.user_id) || (id & 1)) {
                 const std::string mode = args;
                 bool flg = false;
                 std::string res = "avaliable modes:";
                 for (std::string u : modes) {
                     res += " " + u;
                     if (u == mode) {
                         flg = true;
                         history[id].clear();
                         pre_default[id] = mode;
                         conf.p->cq_send("change done.", conf);
                         break;
                     }
                 }
                 if (!flg) {
                     conf.p->cq_send(res, conf);
                 }
             }
             else {
                 conf.p->cq_send("Not on op list.", conf);
             }
             save_history(id);
             return true;
         }},
        {".sw",
         [&]() {
             if (conf.p->is_op(conf.user_id)) {
                 is_open = !is_open;
                 close_message = args;
                 conf.p->cq_send("is_open: " + std::to_string(is_open), conf);
             }
             else {
                 conf.p->cq_send("Not on op list.", conf);
             }
             return true;
         }},
        {".debug",
         [&]() {
             if (conf.p->is_op(conf.user_id)) {
                 is_debug = !is_debug;
                 conf.p->cq_send("is_debug: " + std::to_string(is_debug), conf);
             }
             else {
                 conf.p->cq_send("Not on op list.", conf);
             }
             return true;
         }},
        {".set",
         [&]() {
             std::string reply = "Not on op list.";
             if (conf.p->is_op(conf.user_id)) {
                 std::string type;
                 int64_t num = 0;
                 std::istringstream arg_iss(args);
                 if (!(arg_iss >> type >> num)) {
                     conf.p->cq_send("Unknown type", conf);
                     save_file();
                     return true;
                 }
                 if (type == "reply") {
                     MAX_REPLY = num;
                     reply = "set MAX_REPLY to " + std::to_string(num);
                 }
                 else if (type == "token") {
                     MAX_TOKEN = num;
                     reply = "set MAX_TOKEN to " + std::to_string(num);
                 }
                 else if (type == "red") {
                     RED_LINE = num;
                     reply = "set RED_LINE to " + std::to_string(num);
                 }
                 else {
                     reply = "Unknown type";
                 }
             }
             conf.p->cq_send(reply, conf);
             save_file();
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
    if (is_lock[keyid]) {
        conf.p->cq_send("请等待上次输入的回复。", conf);
        return;
    }
    if (!is_open) {
        conf.p->cq_send("已关闭。" + close_message, conf);
        return;
    }
    if (history.find(id) == history.end()) {
        pre_default[id] = default_prompt;
    }
    std::lock_guard<std::mutex> lock(gptlock[keyid]);
    is_lock[keyid] = true;
    Json::Value J, user_input_J, ign;
    user_input_J["role"] = "user";
    std::string nickname = get_stranger_name(conf.p, conf.user_id);
    user_input_J["content"] = "[User: " + std::to_string(conf.user_id) + "(" +
                              nickname + ")] " + message;
    // J = history[id];
    // while(getlength(J) > MAX_TOKEN - MAX_REPLY){
    //     J.removeIndex(0, &ign);
    //     J.removeIndex(0, &ign);
    // }

    // history[id] = J;
    // J = Json::Value();
    J["model"] = model_name;
    Json::Value K = mode_prompt[pre_default[id]];
    auto it = history.find(id);
    if (it != history.end()) {
        Json::ArrayIndex sz = it->second.size();
        for (Json::ArrayIndex i = 0; i < sz; i++) {
            K.append(it->second[i]);
        }
    }
    K.append(user_input_J);
    J["messages"] = K;
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
    is_lock[keyid] = false;
    if (J.isMember("error")) {
        if (J["error"]["message"].asString().find(
                "This model's maximum context length is") == 0) {
            conf.p->cq_send("Openai ERROR: history message is too long. Please "
                            "try again or try .ai.reset",
                            conf);
            history[id].removeIndex(0, &ign);
            history[id].removeIndex(0, &ign);
            save_history(id);
        }
        else {
            conf.p->cq_send("Openai ERROR: " + J["error"]["message"].asString(),
                            conf);
        }
    }
    else {
        std::string aimsg = J["choices"][0]["message"]["content"].asString();
        aimsg = do_black(aimsg);

        int tokens = static_cast<int>(J["usage"]["total_tokens"].asInt64());

        if (MAX_TOKEN < tokens) {
            history[id].clear();
        }
        else {
            for (int i = 5; i >= 1; i--) {
                if (MAX_TOKEN - RED_LINE / i < tokens) {
                    for (int j = 0; j < i; j++) {
                        history[id].removeIndex(0, &ign);
                        history[id].removeIndex(0, &ign);
                    }
                    break;
                }
            }
        }

        std::string usage = "\n" + J["usage"].toStyledString();
        J.clear();
        J["role"] = "assistant";
        J["content"] = aimsg;
        if (is_debug)
            aimsg += usage;
        conf.p->cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                            "] " + aimsg,
                        conf);
        history[id].append(user_input_J);
        history[id].append(J);
    }
    save_history(id);
}
bool gpt3_5::check(std::string message, const msg_meta &conf)
{
<<<<<<< HEAD
    size_t pos = message.find(".ai");
    if (pos != std::string::npos) {
        if (pos == 0 || message[pos - 1] == ' ' || message[pos - 1] == ']')
            return true;
    }
    if (message.find("你说的话我不喜欢") != std::string::npos &&
        message.find("[CQ:reply,id=") != std::string::npos)
        return true;
    return false;
}
std::string gpt3_5::help()
{
    return "Openai gpt3.5: start with .ai\n"
           "Reading forwarded messages: Reply to a merged record with .ai .fw "
           "[requirement]\n"
           "Recall: Reply to AI message with '你说的话我不喜欢' to recall it.";
=======
    (void)conf;
    return cmd_match_prefix(message, {".ai"});
>>>>>>> 940e730c8833c0a4b2562e8db11f14350afb6c8b
}

DECLARE_FACTORY_FUNCTIONS(gpt3_5)