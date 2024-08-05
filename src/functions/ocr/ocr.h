#pragma once

#include "processable.h"

class ocr : public processable {
public:
    std::string ocr_tostring(const Json::Value &J);
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

DECLARE_FACTORY_FUNCTIONS_HEADER