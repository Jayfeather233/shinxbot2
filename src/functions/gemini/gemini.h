#pragma once

#include "processable.h"
#include <map>
#include <vector>

class gemini : public processable {
private:
    std::vector<std::string> keys;
    std::vector<std::string>::iterator nowkey;
    std::string default_prompt;
    std::vector<std::string> modes;
    std::map<std::string, Json::Value> mode_prompt;

    std::map<uint64_t, Json::Value> history[2];

public:
    gemini();
    void shrink_prompt_size(uint64_t id, bool is_vision);
    std::string generate_text(std::string message, uint64_t id, uint64_t user_id);
    std::string generate_image(std::string message, uint64_t id, uint64_t user_id);
    size_t get_tokens(const Json::Value &history);
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER