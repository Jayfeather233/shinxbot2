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
#ifdef DECLARE_FACTORY_FUNCTIONS
    #undef DECLARE_FACTORY_FUNCTIONS
#endif
#define DECLARE_FACTORY_FUNCTIONS(DerivedClass)                                \
    extern "C" eventprocess *create_t() { return new DerivedClass(); }         \
    extern "C" void destroy_t(eventprocess *p) { delete p; }

#ifdef DECLARE_FACTORY_FUNCTIONS_HEADER
    #undef DECLARE_FACTORY_FUNCTIONS_HEADER
#endif
#define DECLARE_FACTORY_FUNCTIONS_HEADER                                       \
    extern "C" eventprocess *create_t();                                       \
    extern "C" void destroy_t(eventprocess *);
