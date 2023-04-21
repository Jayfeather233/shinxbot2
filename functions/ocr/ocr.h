#pragma once

#include "processable.h"

class ocr : public processable {
public:
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};
