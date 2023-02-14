#pragma once

#include "processable.h"

class auto114 : public processable{
public:
    std::string process(std::string message, std::string message_type, long user_id, long group_id);
    bool check(std::string message, std::string message_type, long user_id, long group_id);
    std::string help();
};