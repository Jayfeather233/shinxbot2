#pragma once

#include "eventprocess.h"

#include <set>
#include <chrono>
#include <mutex>

class poke : public eventprocess {
private:
    std::set<userid_t> no_poke_users;
    std::chrono::steady_clock::time_point lastExecutionTime_{};
    std::mutex mutex_;
    std::chrono::duration<int64_t> minInterval;
public:
    poke();
    void process(bot *p, Json::Value J);
    bool check(bot *p, Json::Value J);
};

extern "C" eventprocess* create();