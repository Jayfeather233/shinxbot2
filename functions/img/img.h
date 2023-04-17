#pragma once

#include "processable.h"
#include <map>
#include <set>

class img : public processable {
private:
    std::map<std::string, int64_t> images;
    std::map<int64_t, bool> op_list;
    std::map<int64_t, bool> is_adding;
    std::map<int64_t, std::string> add_name;
    
    std::map<int64_t, bool> is_deling;
    std::map<int64_t, std::string> del_name;

    std::map<int64_t, Json::Value> belongs;
    std::set<std::string> default_img;
public:
    img();
    void save();
    void del_all(std::string name);
    void del_single(std::string name, int index);
    void add_image(std::string name, std::string image, int64_t group_id);
    void commands(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};