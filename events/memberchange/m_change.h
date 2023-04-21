#pragma once

#include "eventprocess.h"

class m_change : public eventprocess {
public:
    void process(Json::Value J);
    bool check(Json::Value J);
};
