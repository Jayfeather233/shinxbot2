#pragma once

#include "processable.h"
#include <jsoncpp/json/json.h>
#include <map>
#include <set>

class e621 : public processable{
private:
    std::string username, authorkey;
    std::map<int64_t, bool> group, user, admin;
    std::set<std::string> n_search;

    std::string deal_input(const std::string &input, bool is_pool);
    void admin_set(const std::string &input, const std::string message_type, int64_t user_id, int64_t group_id, bool flg);
    std::string get_image_tags(const Json::Value &J);
    std::string get_image_info(const Json::Value &J, int count, bool poolFlag, int retry);
    void save();
public:
    e621();
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};