#pragma once

#include "processable.h"

class r_color : public processable {
public:
    r_color();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

DECLARE_FACTORY_FUNCTIONS_HEADER