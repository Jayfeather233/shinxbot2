#pragma once

#include <jsoncpp/json/json.h>
#include <string>

class eventprocess {
public:
    /**
     * Json data: see at https://docs.go-cqhttp.org/event/
    */
    virtual void process(Json::Value J) = 0;
    virtual bool check(Json::Value J) = 0;
    virtual ~eventprocess() {}
};
