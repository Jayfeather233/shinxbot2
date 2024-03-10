#pragma once

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

    void start_recover();

public:
    heartBeat(const std::vector<std::string> &commands);
    heartBeat() = delete;
    heartBeat(heartBeat &) = delete;
    heartBeat(heartBeat &&) = delete;
    heartBeat operator=(heartBeat) = delete;
    void run();
    void inform();
};