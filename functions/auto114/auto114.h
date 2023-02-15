#pragma once

#include "processable.h"

class auto114 : public processable{
private:
    struct info{
        int num;
        std::string ans;
    }ai[525];
    int len;

    int find_min(long long input);
    std::string getans(long long input);
public:
    auto114();
    std::string process(std::string message, std::string message_type, long user_id, long group_id);
    bool check(std::string message, std::string message_type, long user_id, long group_id);
    std::string help();
};