#pragma once

#include "processable.h"
#include <jsoncpp/json/json.h>
#include <map>
#include <set>

class e621 : public processable{
private:
    std::string username, authorkey;
    std::map<int64_t, bool> group, user;
    std::set<std::string> n_search;

    std::string deal_input(const std::string &input, bool is_pool);
    void admin_set(shinx_message msg, bool flg);
    std::string get_image_tags(const Json::Value &J);
    std::string get_image_info(const Json::Value &J, size_t count, bool poolFlag, int retry, int64_t group_id);
    void save();
public:
    e621();
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};