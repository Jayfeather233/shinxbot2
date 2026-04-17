#pragma once

#include "processable.h"

class dice : public processable {
public:
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER
