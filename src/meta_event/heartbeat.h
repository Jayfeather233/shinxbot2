#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <vector>

class heartBeat {
private:
    std::vector<std::string> recover_commands;

    bool is_recorded;
    bool recovering;
    std::time_t las_time;
    std::atomic<bool> running_{true};

    void start_recover();

public:
    explicit heartBeat(const std::vector<std::string> &commands);
    heartBeat() = delete;
    heartBeat(heartBeat &) = delete;
    heartBeat(heartBeat &&) = delete;
    heartBeat operator=(heartBeat) = delete;
    void run();
    void stop();
    void inform();
};