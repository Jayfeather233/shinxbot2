#pragma once

#include "processable.h"
#include <map>
#include <vector>

class warn_recorder : public processable {
private:
    std::map<groupid_t, std::map<userid_t, std::vector<std::string>>> warns;
    void load_config();
    void save();
    void process_command(std::string command, const msg_meta &conf);

public:
    warn_recorder();

    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    bool reload(const msg_meta &conf) override;
    std::string help() override;
    std::string help(const msg_meta &conf, help_level_t level) override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER