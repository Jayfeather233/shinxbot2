#pragma once

#include "eventprocess.h"

class friendadd : public eventprocess {
public:
    void process(bot *p, Json::Value J);
    bool check(bot *p, Json::Value J);
};

DECLARE_FACTORY_FUNCTIONS_HEADER