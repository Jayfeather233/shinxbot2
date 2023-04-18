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
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};