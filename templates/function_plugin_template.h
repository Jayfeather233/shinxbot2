#pragma once

#include "processable.h"

class function_plugin_template : public processable {
public:
    function_plugin_template() = default;

    bool check(std::string message, const msg_meta &conf) override;
    void process(std::string message, const msg_meta &conf) override;

    std::string help() override;
    std::string help(const msg_meta &conf,
                     help_level_t level = help_level_t::public_only) override;

    bool reload(const msg_meta &conf) override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER
