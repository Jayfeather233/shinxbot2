#pragma once

#include <string>

class processable {
public:
    /**
     * process the message
     * message: message that bot received.
     * message_type: "group" or "private"
     * user_id & group_id
    */
    virtual void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id) = 0;
    /**
     * check if the message need to be process
    */
    virtual bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id) = 0;
    virtual std::string help() = 0;
};