#pragma once

#include "eventprocess.h"

class poke : public eventprocess {
public:
    void process(Json::Value J);
    bool check(Json::Value J);
};
