#pragma once

#include "processable.h"
#include <map>

class img : public processable {
private:
    std::map<std::string, int64_t> images;
    std::map<int64_t, bool> is_adding;
    std::map<int64_t, std::string> add_name;
public:
    img();
    void save();
    void add_image(std::string name, std::string image);
    void commands(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};