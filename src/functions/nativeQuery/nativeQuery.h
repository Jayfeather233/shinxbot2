#pragma once

#include "processable.h"

class nativeQuery : public processable {
public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    std::string help(const msg_meta &conf, help_level_t level);
};

DECLARE_FACTORY_FUNCTIONS_HEADER
