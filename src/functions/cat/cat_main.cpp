#include "cat.h"

#include <filesystem>
#include <fstream>
#include <iostream>

Json::Value catmain::cat_text;

catmain::catmain()
{
    std::string ans = readfile("./data/cat.json");
    if (ans != "") {
        Json::Value J = string_to_json(ans);
        cat_text = J;
    }
    else {
        set_global_log(LOG::ERROR, "Missing file: ./data/cat.json");
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
        conf.p->cq_send("An interactive cat!\n"
                        "First use adopt to have one.\n"
                        "Then you can play, feed and so on!(start with[cat])",
                        conf);
        return;
    }
    if (message.find(".adopt") == 13) {
        auto it = cat_map.find(conf.user_id);
        if (it != cat_map.end()) {
            conf.p->cq_send("You already have one!", conf);
            return;
        }
        else {
            std::string name = trim(message.substr(19));
            if (name.length() <= 0) {
                conf.p->cq_send("Please give it a name", conf);
                return;
            }
            else {
                Cat newcat(name, conf.user_id);
                cat_map[conf.user_id] = newcat;
                conf.p->cq_send(newcat.adopt(), conf);
                save_map();
            }
        }
    }
    else if (message.find(".rename") == 13) {
        auto it = cat_map.find(conf.user_id);
        if (it == cat_map.end()) {
            conf.p->cq_send("You don't have one!", conf);
            return;
        }
        else {
            std::string name = trim(message.substr(20));
            if (name.length() <= 0) {
                conf.p->cq_send("Please give it a name", conf);
                return;
            }
            else {
                conf.p->cq_send(it->second.rename(name), conf);
                save_map();
            }
        }
    }
    else {
        auto it = cat_map.find(conf.user_id);
        if (it == cat_map.end()) {
            conf.p->cq_send("You don't have one!\n"
                            "Use [cat].adopt name to get a cute cat!",
                            conf);
            return;
        }
        else {
            conf.p->cq_send(it->second.process(message), conf);
        }
    }
}
bool catmain::check(std::string message, const msg_meta &conf)
{
    return message.find("&#91;cat&#93;") == 0;
}
std::string catmain::help() { return "online cat. [cat].help"; }
