#pragma once

#include "processable.h"
#include <mutex>

class auto_approve : public processable {
private:
    std::mutex mutex_;
    bool auto_friend_ = true;
    bool auto_group_invite_ = true;

    void load_config();
    void save_config() const;
    bool parse_switch_value(const std::string &value, bool &state);
    std::string state_to_text(bool state) const;

public:
    auto_approve();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    std::string help(const msg_meta &conf, help_level_t level);
};

DECLARE_FACTORY_FUNCTIONS_HEADER
