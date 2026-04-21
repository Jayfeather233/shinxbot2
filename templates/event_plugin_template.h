#pragma once

#include "eventprocess.h"

class event_plugin_template : public eventprocess {
public:
    event_plugin_template() = default;

    bool check(bot *p, Json::Value J) override;
    void process(bot *p, Json::Value J) override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER
