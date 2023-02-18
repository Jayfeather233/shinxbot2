#pragma once

#include "processable.h"
#include <map>

class e621 : public processable{
private:
    std::string username, authorkey;
    std::map<int64_t, bool> group, user, admin, n_search;

    std::string deal_input(std::string input, bool is_pool);
    void admin_set(std::string input, int64_t user_id, int64_t group_id);
    void admin_del(std::string input, int64_t user_id, int64_t group_id);
    std::string get_image_tags(Json::Value J);
    std::string get_image_info(Json::Value J, int retry);
public:
    e621();
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};