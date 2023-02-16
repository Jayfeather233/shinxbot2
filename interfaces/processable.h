#pragma once

#include <string>

class processable {
public:
    virtual void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id) = 0;
    virtual bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id) = 0;
    virtual std::string help() = 0;
};