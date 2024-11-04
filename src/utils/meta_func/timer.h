#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>
#include "bot.h"

class Timer {
private:
    std::chrono::duration<double> interval;
    std::atomic<bool> running;
    std::thread timerThread;
    std::map<std::string, std::vector<std::function<void(bot *p)>>> callbacks;
    bot *p;

    void run();

public:
    Timer(std::chrono::duration<double> dur, bot *p);

    void set_interval(std::chrono::duration<double> dur);
    void add_callback(const std::string &name, std::function<void(bot *p)> cb);
    void remove_callback(const std::string &name);

    void timer_start();
    void timer_stop();

    ~Timer();
};