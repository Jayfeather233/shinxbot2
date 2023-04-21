#pragma once

#include "processable.h"
#include "utils.h"

class recall : public processable {
public:
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};
