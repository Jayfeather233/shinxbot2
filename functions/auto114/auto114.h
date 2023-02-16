#pragma once

#include "processable.h"

class auto114 : public processable{
private:
    struct info{
        int num;
        std::string ans;
    }ai[525];
    int len;

    int find_min(int64_t input);
    std::string getans(int64_t input);
public:
    auto114();
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};