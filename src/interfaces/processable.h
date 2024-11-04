#pragma once

#include "utils.h"
#include <string>

class processable {
public:
    /**
     * msg_meta: see at 'utils.h'
     * process the message
     */
    virtual void process(std::string message, const msg_meta &conf) = 0;
    /**
     * check if the message need to be process
     */
    virtual bool check(std::string message, const msg_meta &conf) = 0;
    virtual std::string help() = 0;
    virtual ~processable() {}

    virtual bool is_support_messageArr() { return false; }
    virtual void process(Json::Value message, const msg_meta &conf)
    {
        throw std::logic_error("Called a function that does not exist");
    }
    virtual bool check(Json::Value message, const msg_meta &conf)
    {
        throw std::logic_error("Called a function that does not exist");
    }
    virtual void
    set_callback(std::function<void(std::function<void(bot *p)>)> f)
    {
    }
    virtual void set_backup_files(archivist *p, const std::string &name) {}
};

#ifdef DECLARE_FACTORY_FUNCTIONS
    #undef DECLARE_FACTORY_FUNCTIONS
#endif
#define DECLARE_FACTORY_FUNCTIONS(DerivedClass)                                \
    extern "C" processable *create_t() { return new DerivedClass(); }          \
    extern "C" void destroy_t(processable *p) { delete p; }

#ifdef DECLARE_FACTORY_FUNCTIONS_HEADER
    #undef DECLARE_FACTORY_FUNCTIONS_HEADER
#endif
#define DECLARE_FACTORY_FUNCTIONS_HEADER                                       \
    extern "C" processable *create_t();                                        \
    extern "C" void destroy_t(processable *);