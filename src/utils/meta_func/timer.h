#pragma once

#include "bot.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class Timer {
private:
    std::chrono::duration<double> interval;
    std::atomic<bool> running;
    std::thread timerThread;
    std::mutex callbacks_mutex;
    std::map<std::string, std::vector<std::function<void(bot *p)>>> callbacks;
    bot *p;

    void run();

public:
    Timer(std::chrono::duration<double> dur, bot *p);

    void set_interval(std::chrono::duration<double> dur);
    void add_callback(const std::string &name, std::function<void(bot *p)> cb);
    void remove_callback(const std::string &name);
    bool is_running() const { return running.load(); }

    void timer_start();
    void timer_stop();

    ~Timer();
};