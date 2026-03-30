#pragma once

#include "processable.h"

#include <map>

class RP : public processable {
private:
    std::map<userid_t, std::pair<uint32_t, std::string>>
        reply_content; // user_id -> <possible, messsage>
public:
    RP();
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    bool reload(const msg_meta &conf) override;
    std::string help() override;
    std::string help(const msg_meta &conf, help_level_t level) override;
    void save();
};

DECLARE_FACTORY_FUNCTIONS_HEADER