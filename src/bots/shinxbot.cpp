#include "shinxbot.hpp"
#include "dynamic_lib.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <filesystem>
#include <httplib.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <fmt/core.h>

namespace fs = fs;

void shinxbot::read_server_message(int new_socket) {}
int shinxbot::start_server()
{
    httplib::Server svr;

    svr.Post("/", [&](const httplib::Request &req, httplib::Response &res) {
        std::string s_buffer = req.body;
        std::thread([this, s_buffer]() {
            std::string *u = new std::string(s_buffer);
            input_process(u);
        }).detach();

        res.set_content("{}", "application/json");
    });

    fmt::print("Server is starting on port {}...", receive_port);
    return svr.listen("0.0.0.0", receive_port);
}

void shinxbot::refresh_log_stream()
{
    std::time_t nt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *std::localtime(&nt);

    std::string formatted_log = fmt::format("./log/{}/{:04}_{:02}_{:02}", 
                                            botqq, 
                                            tt.tm_year + 1900, 
                                            tt.tm_mon + 1, 
                                            tt.tm_mday);

    if (!fs::exists(formatted_log.c_str())) {
        fs::create_directories(formatted_log.c_str());
    }
    for (int i = 0; i < 3; i++) {
        if (LOG_output[i].is_open()) {
            LOG_output[i].close();
        }
    }
    LOG_output[0] = std::ofstream(formatted_log + "/info.log", std::ios_base::app);
    LOG_output[1] = std::ofstream(formatted_log + "/warn.log", std::ios_base::app);
    LOG_output[2] = std::ofstream(formatted_log + "/erro.log", std::ios_base::app);
}

template <typename T> void close_dl(void *handle, T *p)
{
    typedef void (*close_t)(T *);
    close_t closex = (close_t)dlsym(handle, "destroy_t");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        set_global_log(LOG::WARNING,
                       std::string("Cannot load symbol 'destroy_t': ") +
                           dlsym_error);
        // delete p; // This is not always safe
    }
    else {
        closex(p);
    }
    dlclose(handle);
}

void shinxbot::unload_func(std::tuple<processable *, void *, std::string> &f)
{
    this->mytimer->remove_callback(std::get<2>(f));
    this->archive->remove_path(std::get<2>(f));

    close_dl(std::get<1>(f), std::get<0>(f));
}
void shinxbot::unload_func(std::tuple<eventprocess *, void *, std::string> &f)
{
    this->mytimer->remove_callback(std::get<2>(f));
    this->archive->remove_path(std::get<2>(f));
    close_dl(std::get<1>(f), std::get<0>(f));
}

void shinxbot::init_func(const std::string &name, processable *p)
{
    p->set_callback([&](std::function<void(bot * p)> func) {
        this->mytimer->add_callback(name, func);
    });
    p->set_backup_files(this->archive, name);
}
void shinxbot::init_func(const std::string &name, eventprocess *p) {}

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

    Json::Value J_op = string_to_json(readfile("./config/op_list.json", "[]"));
    parse_json_to_set(J_op, op_list);

    Json::Value J_rec =
        string_to_json(readfile("./config/recover.json", "{\"commands\":[]}"));
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
    this->archive->add_path("MAIN", "./config");
    this->archive->set_default_pwd(std::to_string(this->botqq));
}

shinxbot::shinxbot(int recv_port, int send_port) : bot(recv_port, send_port) {}
shinxbot::shinxbot(const Json::Value &J) : bot(J["recv_port"].asInt(), J["send_port"].asInt()) {}

bool shinxbot::is_op(const userid_t a) const
{
    return op_list.find(a) != op_list.end();
}

bool shinxbot::meta_func(std::string message, const msg_meta &conf)
{
    if (message == "bot.help") {
        std::string help_message;
        for (auto funcx : functions) {
            processable *func = std::get<0>(funcx);
            if (func->help() != "")
                help_message += func->help() + '\n';
        }
        help_message += "本Bot项目地址：https://github.com/"
                        "Jayfeather233/shinxbot2";
        cq_send(help_message, conf);
        return false;
    }
    else if (message == "bot.off" && is_op(conf.user_id)) {
        bot_isopen = false;
        cq_send("isopen=" + std::to_string(bot_isopen), conf);
        return false;
    }
    else if (message == "bot.on" && is_op(conf.user_id)) {
        bot_isopen = true;
        cq_send("isopen=" + std::to_string(bot_isopen), conf);
        return false;
    }
    else if (message == "bot.backup" && is_op(conf.user_id)) {
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
    }
    else if (message.find("bot.load") == 0 && is_op(conf.user_id)) {
        std::istringstream iss(message.substr(8));
        std::ostringstream oss;
        bool flg = true;
        std::string type, name;
        iss >> type;
        if (type == "function") {
            while (iss >> name) {
                for (size_t i = 0; i < functions.size(); ++i) {
                    if (std::get<2>(functions[i]) == name) {
                        unload_func(functions[i]);

                        auto u = load_function<processable>(
                            "./lib/functions/lib" + name + ".so");
                        if (u.first != nullptr) {
                            std::get<0>(functions[i]) = u.first;
                            std::get<1>(functions[i]) = u.second;
                            init_func(name, u.first);
                            oss << "reload " << name << std::endl;
                        }
                        else {
                            functions.erase(functions.begin() + i);
                            oss << "load " << name << " failed" << std::endl;
                        }
                        flg = false;
                        break;
                    }
                }
            }
            if (flg) {
                auto u = load_function<processable>("./lib/functions/lib" +
                                                    name + ".so");
                if (u.first != nullptr) {
                    functions.push_back(
                        std::make_tuple(u.first, u.second, name));
                    init_func(name, u.first);
                    oss << "load " << name << std::endl;
                }
                else {
                    oss << "load " << name << " failed" << std::endl;
                }
            }
            cq_send(trim(oss.str()), conf);
            return false;
        }
        else if (type == "event") {
            while (iss >> name) {
                for (size_t i = 0; i < events.size(); ++i) {
                    if (std::get<2>(events[i]) == name) {
                        unload_func(events[i]);

                        auto u = load_function<eventprocess>(
                            "./lib/events/lib" + name + ".so");
                        if (u.first != nullptr) {
                            std::get<0>(events[i]) = u.first;
                            std::get<1>(events[i]) = u.second;
                            init_func(name, u.first);
                            oss << "reload " << name << std::endl;
                        }
                        else {
                            events.erase(events.begin() + i);
                            oss << "load " << name << " failed" << std::endl;
                        }
                        flg = false;
                        break;
                    }
                }
            }
            if (flg) {
                auto u = load_function<eventprocess>("./lib/events/lib" + name +
                                                     ".so");
                if (u.first != nullptr) {
                    events.push_back(std::make_tuple(u.first, u.second, name));
                    init_func(name, u.first);
                    oss << "load " << name << std::endl;
                }
                else {
                    oss << "load " << name << " failed" << std::endl;
                }
            }
            cq_send(trim(oss.str()), conf);
            return false;
        }
        else {
            cq_send("useage: bot.load [function|event] name", conf);
            return false;
        }
    }
    else if (message == "bot.list_module") {
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
    }
    else if (message.find("bot.unload") == 0 && is_op(conf.user_id)) {
        std::istringstream iss(message.substr(10));
        std::ostringstream oss;
        std::string type, name;
        iss >> type;
        if (type == "function") {
            while (iss >> name) {
                bool flg = true;
                for (size_t i = 0; i < functions.size(); ++i) {
                    if (std::get<2>(functions[i]) == name) {
                        unload_func(functions[i]);
                        functions.erase(functions.begin() + i);
                        oss << "unload " << name << std::endl;
                        flg = false;
                        break;
                    }
                }
                if (flg)
                    oss << name << " not found" << std::endl;
            }
            cq_send(trim(oss.str()), conf);
            return false;
        }
        else if (type == "event") {
            while (iss >> name) {
                bool flg = true;
                for (size_t i = 0; i < events.size(); ++i) {
                    if (std::get<2>(events[i]) == name) {
                        unload_func(events[i]);
                        events.erase(events.begin() + i);
                        oss << "unload " << name << std::endl;
                        flg = false;
                        break;
                    }
                }
                if (flg)
                    oss << name << " not found" << std::endl;
            }
            cq_send(trim(oss.str()), conf);
            return false;
        }
        else {
            cq_send("useage: bot.unload [function|event] name", conf);
            return false;
        }
    } else if (message == "bot.progress") {
        cq_send(descBar(), conf);
        return false;
    }
    else
        return true;
}

void shinxbot::input_process(std::string *input)
{
    if (*input == "") {
        delete input;
        return;
    }
    Json::Value J = string_to_json(*input);
    delete input;
    std::string post_type = J["post_type"].asString();

    if ((post_type == "request" || post_type == "notice") && bot_isopen) {
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
            int64_t message_id = J["message_id"].asInt64();
            std::string message_type = J["message_type"].asString();
            if (message_type == "group" || message_type == "private") {
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
                if (meta_func(messageStr, conf) && bot_isopen) {
                    for (auto funcx : functions) {
                        processable *func = std::get<0>(funcx);
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
                        catch (char *e) {
                            cq_send((std::string) "Throw an char*: " + e, conf);
                            setlog(LOG::ERROR,
                                   (std::string) "Throw an char*: " + e);
                        }
                        catch (std::string e) {
                            cq_send("Throw an string: " + e, conf);
                            setlog(LOG::ERROR, "Throw an string: " + e);
                        }
                        catch (std::exception &e) {
                            cq_send((std::string) "Throw an exception: " +
                                        e.what(),
                                    conf);
                            setlog(LOG::ERROR,
                                   (std::string) "Throw an exception: " +
                                       e.what());
                        }
                        catch (...) {
                            cq_send("Throw an unknown error", conf);
                            setlog(LOG::ERROR, "Throw an unknown error");
                        }
                    }
                }
            }
        }
    }
    else if (post_type == "meta_event") {
        if (J["meta_event_type"].asString() == "heartbeat") {
            recorder->inform();
        }
    }
}

void shinxbot::run()
{
    this->mytimer =
        new Timer(std::chrono::milliseconds(500), this); // smallest time: 1s
    this->archive = new archivist();

    try {
        for (const auto &entry : fs::directory_iterator("./lib/functions/")) {
            if (entry.is_regular_file() || entry.is_symlink()) {
                std::string filename = entry.path().filename().string();
                if (filename.size() > 3 &&
                    filename.substr(filename.size() - 3) == ".so") {
                    auto result = load_function<processable>(entry.path());
                    if (result.first != nullptr) {
                        if (filename.find("lib") == 0) {
                            filename = filename.substr(3);
                        }
                        filename = filename.substr(0, filename.length() - 3);
                        functions.push_back(std::make_tuple(
                            result.first, result.second, filename));
                        set_global_log(LOG::INFO,
                                       "Loaded function: " + filename);
                    }
                    else
                        set_global_log(LOG::ERROR,
                                       "Error while loading function:" +
                                           filename);
                }
            }
        }
    }
    catch (const fs::filesystem_error &ex) {
        set_global_log(LOG::ERROR,
                       std::string("Error accessing directory: ") + ex.what());
    }

    try {
        for (const auto &entry : fs::directory_iterator("./lib/events/")) {
            if (entry.is_regular_file() || entry.is_symlink()) {
                std::string filename = entry.path().filename().string();
                if (filename.size() > 3 &&
                    filename.substr(filename.size() - 3) == ".so") {
                    auto result = load_function<eventprocess>(entry.path());
                    if (result.first != nullptr) {
                        if (filename.find("lib") == 0) {
                            filename = filename.substr(3);
                        }
                        filename = filename.substr(0, filename.length() - 3);
                        events.push_back(std::make_tuple(
                            result.first, result.second, filename));
                        setlog(LOG::INFO, "Loaded event: " + filename);
                    }
                    else
                        set_global_log(LOG::ERROR,
                                       "Error while loading function:" +
                                           filename);
                }
            }
        }
    }
    catch (const fs::filesystem_error &ex) {
        set_global_log(LOG::ERROR,
                       std::string("Error accessing directory: ") + ex.what());
    }

    this->init();

    cq_send_all_op("Love you!");
    std::thread(&heartBeat::run, recorder).detach();

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

void shinxbot::setlog(LOG type, std::string message)
{
    std::lock_guard<std::mutex> lock(log_lock);

    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);

    if (!(tt.tm_year == last_getlog.tm_year &&
          tt.tm_mon == last_getlog.tm_mon &&
          tt.tm_mday == last_getlog.tm_mday)) {
        last_getlog = tt;
        this->refresh_log_stream();
    }

    std::string formatted_message =
        fmt::format("[{:02}:{:02}:{:02}][{}] {}\n", tt.tm_hour, tt.tm_min,
                    tt.tm_sec, LOG_name[type], message);

    if (type == LOG::ERROR)
        fmt::print(stderr, formatted_message);
    else
        fmt::print(formatted_message);
    LOG_output[type] << formatted_message;
    LOG_output[type].flush();
}

void shinxbot::cq_send_all_op(const std::string &u)
{
    msg_meta conf = (msg_meta){"private", 0, 0, 0, this};
    for (userid_t uid : op_list) {
        conf.user_id = uid;
        cq_send(u, conf);
    }
}

shinxbot::~shinxbot()
{
    bot_is_on = false;
    this->mytimer->timer_stop();
    delete this->mytimer;
    delete this->archive;
    delete this->recorder;

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