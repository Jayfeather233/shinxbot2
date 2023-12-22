#pragma once

#include "processable.h"
#include <vector>
#include <map>

class gemini : public processable{
private:
    std::vector<std::string> keys;
    std::vector<std::string>::iterator nowkey;
    std::string default_prompt;
    std::vector<std::string> modes;
    std::map<std::string, Json::Value> mode_prompt;

    std::map<int64_t, Json::Value> history[2];
public:
    gemini();
    void shrink_prompt_size(int64_t id, bool is_vision);
    std::string generate_image(std::string message, int64_t id);
    std::string generate_text(std::string message, int64_t id);
    size_t get_tokens(const Json::Value &history);
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
