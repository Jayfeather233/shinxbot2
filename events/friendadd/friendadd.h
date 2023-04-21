#pragma once

#include "eventprocess.h"

class friendadd : public eventprocess {
public:
    void process(Json::Value J);
    bool check(Json::Value J);
};
