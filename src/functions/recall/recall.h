#pragma once

#include "processable.h"
#include "utils.h"

class recall : public processable {
public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
