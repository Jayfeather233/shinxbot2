#pragma once

#include "processable.h"
#include "utils.h"
#include <map>

class gray_list : public processable {
private:
    std::map<long, Json::Value> g_list;
public:
    gray_list();
    void save();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

DECLARE_FACTORY_FUNCTIONS_HEADER