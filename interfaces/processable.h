#pragma once

#include <string>

class processable {
public:
    virtual void process(std::string message, std::string message_type, long user_id, long group_id) = 0;
    virtual bool check(std::string message, std::string message_type, long user_id, long group_id) = 0;
    virtual std::string help() = 0;
};