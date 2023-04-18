#pragma once

#include <string>
#include "utils.h"

class processable {
public:
    /**
     * shinx_message: see at 'utils.h'
     * process the message
    */
    virtual void process(shinx_message msg) = 0;
    /**
     * check if the message need to be process
    */
    virtual bool check(shinx_message msg) = 0;
    virtual std::string help() = 0;
};