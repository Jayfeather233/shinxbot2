#include "dynamic_lib.hpp"
#include "shinxbot.hpp"

#include <filesystem>
#include <iostream>
#include <thread>

namespace fs = fs;

void shinxbot::init()
{
    for (;;) {
        try {
            Json::Value J = string_to_json(cq_get("get_login_info"));
            botqq = J["data"]["user_id"].asUInt64();
            break;
        }
        catch (...) {
        }
        sleep(10); // 10 sec
    }
    std::cout << "botqq:" << botqq << std::endl;

    refresh_log_stream();

    Json::Value J_op = string_to_json(
        readfile(bot_config_path(this, "core/op_list.json"), "[]"));
    parse_json_to_set(J_op, op_list);

    Json::Value J_block = string_to_json(
        readfile(bot_config_path(this, "core/blocklist.json"), "{}"));
    for (const auto &member : J_block.getMemberNames()) {
        groupid_t gid = std::stoull(member);
        group_blocklist[gid] = blockItem(J_block[member]);
    }

    Json::Value J_rec = string_to_json(readfile(
        bot_config_path(this, "core/recover.json"), "{\"commands\":[]}"));
    Json::Value Ja_rec = J_rec["commands"];
    Json::ArrayIndex sz = Ja_rec.size();
    std::vector<std::string> rec_list;
    for (Json::ArrayIndex i = 0; i < sz; ++i) {
        rec_list.push_back(Ja_rec[i].asString());
    }

    recorder = new heartBeat(rec_list);
    for (auto px : functions) {
        init_func(std::get<2>(px), std::get<0>(px));
    }
    for (auto px : events) {
        init_func(std::get<2>(px), std::get<0>(px));
    }
    this->archive->add_path("MAIN", getConfigDir());
    this->archive->set_default_pwd(std::to_string(this->botqq));
}

void shinxbot::input_process(const std::string &input)
{
    if (input.empty()) {
        return;
    }
    Json::Value J = string_to_json(input);
    if (J.isMember("post_type") == false) {
        return;
    }
    std::string post_type = J["post_type"].asString();

    if ((post_type == "request" || post_type == "notice") && bot_enabled) {
        for (auto evenx : events) {
            eventprocess *even = std::get<0>(evenx);
            if (even->check(this, J)) {
                even->process(this, J);
            }
        }
    }
    else if (post_type == "message") {
        if (J.isMember("message_type") && J.isMember("message")) {
            Json::Value messageArr;
            if (J["message"].isArray()) {
                messageArr = J["message"];
            }
            else if (J["message"].isString()) {
                Json::Value jj;
                jj["type"] = "text";
                jj["data"]["text"] = J["message"];
                messageArr.append(jj);
            }
            else {
                messageArr.append(J["message"]);
            }
            std::string messageStr = messageArr_to_string(J["message"]);
            messageArr = expand_string_to_messageArr(messageStr);
            int64_t message_id = J["message_id"].asInt64();
            std::string message_type = J["message_type"].asString();
            if (message_type == "group" || message_type == "private" ||
                message_type == "internal") {
                userid_t user_id = 0;
                groupid_t group_id = 0;
                if (J.isMember("group_id")) {
                    group_id = J["group_id"].asUInt64();
                }
                if (J.isMember("user_id")) {
                    user_id = J["user_id"].asUInt64();
                }
                msg_meta conf = (msg_meta){message_type, user_id, group_id,
                                           message_id, this};
                if (meta_func(messageStr, conf) && bot_enabled) {
                    for (auto funcx : functions) {
                        processable *func = std::get<0>(funcx);
                        std::string name = std::get<2>(funcx);
                        if (message_type == "group" &&
                            group_blocklist[group_id].is_blocked(name)) {
                            continue;
                        }
                        try {
                            if (func->is_support_messageArr()) {
                                if (func->check(messageArr, conf)) {
                                    func->process(messageArr, conf);
                                }
                            }
                            else {
                                if (func->check(messageStr, conf)) {
                                    func->process(messageStr, conf);
                                }
                            }
                        }
                        catch (const char *e) {
                            cq_send((std::string) "Throw an char*: " + e, conf);
                            setlog(LOG::ERROR, name + ": Throw an char*: " + e);
                        }
                        catch (const std::string &e) {
                            cq_send("Throw an string: " + e, conf);
                            setlog(LOG::ERROR,
                                   name + ": Throw an string: " + e);
                        }
                        catch (std::exception &e) {
                            cq_send((std::string) "Throw an exception: " +
                                        e.what(),
                                    conf);
                            setlog(LOG::ERROR,
                                   name + ": Throw an exception: " + e.what());
                        }
                        catch (...) {
                            cq_send("Throw an unknown error", conf);
                            setlog(LOG::ERROR,
                                   name + ": Throw an unknown error");
                        }
                    }
                }
            }
        }
    }
    else if (post_type == "meta_event") {
        if (J.isMember("meta_event_type") &&
            J["meta_event_type"].asString() == "heartbeat") {
            recorder->inform();
        }
    }
}

void shinxbot::run()
{
    this->mytimer =
        new Timer(std::chrono::milliseconds(500), this); // smallest time: 1s
    this->archive = new archivist();

    load_module_filter_config();

    auto extract_module_name = [](const std::string &filename) -> std::string {
        if (filename.size() <= 3 ||
            filename.substr(filename.size() - 3) != ".so") {
            return "";
        }
        std::string name = filename;
        if (name.find("lib") == 0) {
            name = name.substr(3);
        }
        return name.substr(0, name.length() - 3);
    };

    auto load_all = [&](const fs::path &dir,
                        const std::set<std::string> &enabled,
                        const std::string &kind, auto loader_and_store) {
        try {
            for (const auto &entry : fs::directory_iterator(dir)) {
                if (!(entry.is_regular_file() || entry.is_symlink())) {
                    continue;
                }

                std::string filename = entry.path().filename().string();
                std::string name = extract_module_name(filename);
                if (name.empty()) {
                    continue;
                }

                if (!enabled.empty() && enabled.find(name) == enabled.end()) {
                    set_global_log(LOG::INFO,
                                   "Skipped " + kind +
                                       " by module_load config: " + name);
                    continue;
                }

                if (!loader_and_store(entry.path(), name)) {
                    set_global_log(LOG::ERROR, "Error while loading " + kind +
                                                   ":" + filename);
                }
            }
        }
        catch (const fs::filesystem_error &ex) {
            set_global_log(LOG::ERROR,
                           std::string("Error accessing directory: ") +
                               ex.what());
        }
    };

    load_all("./lib/functions/", enabled_functions, "function",
             [&](const fs::path &path, const std::string &name) {
                 auto result = load_function<processable>(path);
                 if (result.first == nullptr) {
                     return false;
                 }
                 functions.push_back(
                     std::make_tuple(result.first, result.second, name));
                 set_global_log(LOG::INFO, "Loaded function: " + name);
                 return true;
             });

    load_all("./lib/events/", enabled_events, "event",
             [&](const fs::path &path, const std::string &name) {
                 auto result = load_function<eventprocess>(path);
                 if (result.first == nullptr) {
                     return false;
                 }
                 events.push_back(
                     std::make_tuple(result.first, result.second, name));
                 setlog(LOG::INFO, "Loaded event: " + name);
                 return true;
             });

    this->init();

    cq_send_all_op("Love you!");
    heartbeat_thread = std::thread(&heartBeat::run, recorder);

    this->mytimer->timer_start();
    this->start_server();
    this->mytimer->timer_stop();

    for (auto ux : functions) {
        delete std::get<0>(ux);
        dlclose(std::get<1>(ux));
    }
    functions.clear();
    for (auto ux : events) {
        delete std::get<0>(ux);
        dlclose(std::get<1>(ux));
    }
    events.clear();
}
