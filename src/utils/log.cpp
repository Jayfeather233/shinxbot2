#include "bot.h"
#include <ctime>
#include <chrono>
#include <iomanip>
#include <iostream>

std::string LOG_name[3] = {"INFO", "WARNING", "ERROR"};

void set_global_log(LOG type, std::string message){
    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);
    std::ostringstream oss;
    oss << "[" << std::setw(2) << std::setfill('0') << tt.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tt.tm_min << ":" << std::setw(2)
        << std::setfill('0') << tt.tm_sec << "][" << LOG_name[type] << "] "
        << message << std::endl;
    if (type == LOG::ERROR)
        std::cerr << oss.str();
    else
        std::cout << oss.str();
}