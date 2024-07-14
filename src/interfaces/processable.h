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
    virtual void
    set_backup_files(archivist *p, const std::string &name)
    {
    }
};
