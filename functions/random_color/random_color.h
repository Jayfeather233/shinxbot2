#pragma once

#include "processable.h"

#include <cctype>
#include <random>

using u32    = uint_least32_t;
using engine = std::mt19937;

class r_color : public processable {
private:
    engine generator;
    std::uniform_int_distribution< u32 > uni_dis_0_255 = std::uniform_int_distribution< u32 >(0, 255);
public:
    r_color();
    void process(std::string message, std::string message_type, long user_id, long group_id);
    bool check(std::string message, std::string message_type, long user_id, long group_id);
    std::string help();
};