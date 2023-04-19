#pragma once

#include "utils.h"
#include "processable.h"

class recall : public processable {
public:
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};