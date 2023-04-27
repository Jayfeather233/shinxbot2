#include "cat.h"

#include <filesystem>
#include <fstream>
#include <iostream>

Json::Value catmain::cat_text;

catmain::catmain()
{
    std::string ans = readfile("./config/cat.json");
    if (ans != "") {
        Json::Value J = string_to_json(ans);
        cat_text = J;
    }
    else {
        setlog(LOG::ERROR, "Missing file: ./config/cat.json");
    }

    ans = readfile("./config/cats/user_list.json", "[]");
    Json::Value user_list = string_to_json(ans);
    auto sz = user_list.size();
    for (Json::ArrayIndex i = 0; i < sz; ++i) {
        cat_map[user_list[i].asInt64()] = Cat((int64_t)user_list[i].asInt64());
    }
}

void catmain::save_map()
{
    Json::Value J;
    for (auto it : cat_map) {
        J.append(it.first);
    }
    writefile("./config/cats/user_list.json", J.toStyledString());
}

Json::Value catmain::get_text() { return cat_text; }

void catmain::process(std::string message, const msg_meta &conf)
{
    if (message == "&#91;cat&#93;.help") {
        message = "An interactive cat!\n"
                  "First use adopt to have one.\n"
                  "Then you can play, feed and so on!(start with[cat])";
        cq_send(message, conf);
        return;
    }
    if (message.find(".adopt") == 13) {
        auto it = cat_map.find(conf.user_id);
        if (it != cat_map.end()) {
            message = "You already have one!";
            cq_send(message, conf);
            return;
        }
        else {
            std::string name = trim(message.substr(19));
            if (name.length() <= 0) {
                message = "Please give it a name";
                cq_send(message, conf);
                return;
            }
            else {
                Cat newcat(name, conf.user_id);
                cat_map[conf.user_id] = newcat;
                message = newcat.adopt();
                cq_send(message, conf);
                save_map();
            }
        }
    }
    else if (message.find(".rename") == 13) {
        auto it = cat_map.find(conf.user_id);
        if (it == cat_map.end()) {
            message = "You don't have one!";
            cq_send(message, conf);
            return;
        }
        else {
            std::string name = trim(message.substr(20));
            if (name.length() <= 0) {
                message = "Please give it a name";
                cq_send(message, conf);
                return;
            }
            else {
                message = it->second.rename(name);
                cq_send(message, conf);
                save_map();
            }
        }
    }
    else {
        auto it = cat_map.find(conf.user_id);
        if (it == cat_map.end()) {
            message =
                "You don't have one!\nUse [cat].adopt name to get a cute cat!";
            cq_send(message, conf);
            return;
        }
        else {
            message = it->second.process(message);
            cq_send(message, conf);
        }
    }
}
bool catmain::check(std::string message, const msg_meta &conf)
{
    return message.find("&#91;cat&#93;") == 0;
}
std::string catmain::help() { return "online cat. [cat].help"; }
