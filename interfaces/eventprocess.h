#pragma once

#include <string>
#include <jsoncpp/json/json.h>

class eventprocess {
public:
    virtual void process(Json::Value J) = 0;
    virtual bool check(Json::Value J) = 0;
};