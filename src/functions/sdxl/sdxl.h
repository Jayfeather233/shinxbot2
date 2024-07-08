#pragma once

#include "processable.h"

class sdxl : public processable {
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

extern "C" processable* create();