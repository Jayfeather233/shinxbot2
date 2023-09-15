#pragma once

#include "eventprocess.h"

class m_change : public eventprocess {
public:
    void process(bot *p, Json::Value J);
    bool check(bot *p, Json::Value J);
};
