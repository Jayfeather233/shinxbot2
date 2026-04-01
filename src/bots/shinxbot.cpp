#include "shinxbot.hpp"

#include <dlfcn.h>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>

namespace fs = fs;

// ===== Logging =====
void shinxbot::refresh_log_stream()
{
    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *std::localtime(&nt);

    std::string formatted_log =
        fmt::format("./log/{}/{:04}_{:02}_{:02}", botqq, tt.tm_year + 1900,
                    tt.tm_mon + 1, tt.tm_mday);

    if (!fs::exists(formatted_log.c_str())) {
        fs::create_directories(formatted_log.c_str());
    }
    for (int i = 0; i < 3; i++) {
        if (LOG_output[i].is_open()) {
            LOG_output[i].close();
        }
    }
    LOG_output[0] =
        std::ofstream(formatted_log + "/info.log", std::ios_base::app);
    LOG_output[1] =
        std::ofstream(formatted_log + "/warn.log", std::ios_base::app);
    LOG_output[2] =
        std::ofstream(formatted_log + "/erro.log", std::ios_base::app);
}

shinxbot::shinxbot(int recv_port, int send_port, const std::string &tk)
    : bot(recv_port, send_port, tk)
{
}
shinxbot::shinxbot(const Json::Value &J)
    : bot(J["recv_port"].asInt(), J["send_port"].asInt(), J["token"].asString())
{
}

bool shinxbot::is_op(const userid_t a) const
{
    return op_list.find(a) != op_list.end();
}

// ===== Runtime logging and teardown =====
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

void shinxbot::cq_send_all_op(const std::string &message)
{
    msg_meta conf = (msg_meta){"private", 0, 0, 0, this};
    for (userid_t uid : op_list) {
        conf.user_id = uid;
        cq_send(message, conf);
    }
}

shinxbot::~shinxbot()
{
    if (this->mytimer != nullptr) {
        this->mytimer->timer_stop();
        delete this->mytimer;
        this->mytimer = nullptr;
    }
    if (this->archive != nullptr) {
        delete this->archive;
        this->archive = nullptr;
    }
    if (this->recorder != nullptr) {
        this->recorder->stop();
        if (heartbeat_thread.joinable()) {
            heartbeat_thread.join();
        }
        delete this->recorder;
        this->recorder = nullptr;
    }

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