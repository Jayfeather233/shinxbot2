#pragma once

#include "processable.h"

#include <map>

class RP : public processable {
private:
    std::map<userid_t, std::pair<uint32_t, std::string>> reply_content; // user_id -> <possible, messsage>
public:
    RP();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    void save();
};

DECLARE_FACTORY_FUNCTIONS_HEADER