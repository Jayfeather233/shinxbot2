#pragma once

#include "bot.h"
#include <jsoncpp/json/json.h>
#include <string>

class eventprocess {
public:
    /**
     * Json data: see at
     * https://github.com/botuniverse/onebot-11/tree/master/event, except for
     * message.md
     */
    virtual void process(bot *p, Json::Value J) = 0;
    virtual bool check(bot *p, Json::Value J) = 0;
    virtual ~eventprocess() {}
};
