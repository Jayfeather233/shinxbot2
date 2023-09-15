#pragma once

#include "eventprocess.h"

class talkative : public eventprocess {
public:
    void process(bot *p, Json::Value J);
    bool check(bot *p, Json::Value J);
};
