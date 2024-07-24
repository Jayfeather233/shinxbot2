#pragma once

#include "processable.h"
#include <jsoncpp/json/json.h>
#include <map>
#include <set>

class img : public processable {
private:
    std::map<std::string, uint64_t> images;
    std::map<userid_t, bool> is_adding;
    std::map<userid_t, std::string> add_name;

    std::map<userid_t, bool> is_deling;
    std::map<userid_t, std::string> del_name;

    std::map<groupid_t, Json::Value> belongs;
    std::set<std::string> default_img;

public:
    img();
    void save();
    void del_all(std::string name);
    void del_single(std::string name, int index);
    int add_image(std::string name, std::string image, groupid_t group_id);
    void belong_to(std::string name, groupid_t group_id);
    std::string commands(std::string message, const msg_meta &conf);
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    void set_backup_files(archivist *p, const std::string &name);
};

extern "C" processable* create();