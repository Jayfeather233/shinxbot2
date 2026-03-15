#pragma once

#include "processable.h"
#include <map>
#include <vector>

class warn_recorder : public processable {
private:
    std::map<groupid_t, std::map<userid_t, std::vector<std::string>>> warns;
    void save();
    void process_command(std::string command, const msg_meta &conf);
public:
    warn_recorder();

    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

DECLARE_FACTORY_FUNCTIONS_HEADER