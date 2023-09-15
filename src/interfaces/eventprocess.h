#pragma once

#include <jsoncpp/json/json.h>
#include <string>
#include "bot.h"

class eventprocess {
public:
    /**
     * Json data: see at https://docs.go-cqhttp.org/event/
    */
    virtual void process(bot *p, Json::Value J) = 0;
    virtual bool check(bot *p, Json::Value J) = 0;
    virtual ~eventprocess() {}
};
