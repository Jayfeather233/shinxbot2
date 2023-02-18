#pragma once

#include "processable.h"
#include <map>

class e621 : public processable{
private:
    std::string username, authorkey;
    std::map<int64_t, bool> group, user, admin;
    int lasMsg, retry;
public:
    e621();
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};