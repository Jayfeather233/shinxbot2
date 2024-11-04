#include "bot.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fmt/core.h>

std::string LOG_name[3] = {"INFO", "WARNING", "ERROR"};

void set_global_log(LOG type, std::string message)
{
    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *std::localtime(&nt);

    std::string formatted_message =
        fmt::format("[{:02}:{:02}:{:02}][{}] {}\n", tt.tm_hour, tt.tm_min,
                    tt.tm_sec, LOG_name[type], message);

    if (type == LOG::ERROR)
        fmt::print(stderr, formatted_message);
    else
        fmt::print(formatted_message);
}