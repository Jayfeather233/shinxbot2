#pragma once

#include "eventprocess.h"

class talkative : public eventprocess {
public:
    void process(Json::Value J);
    bool check(Json::Value J);
};