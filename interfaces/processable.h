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
};
