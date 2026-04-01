#include "cat.h"

#include <filesystem>
#include <fstream>
#include <iostream>

Json::Value catmain::cat_text;

catmain::catmain()
{
    const std::string cat_text_path =
        bot_resource_path(nullptr, "cat/cat_text.json");
    std::string ans = readfile(cat_text_path);
    if (ans != "") {
        Json::Value J = string_to_json(ans);
        cat_text = J;
    }
    else {
        set_global_log(LOG::ERROR, "Missing file: " + cat_text_path);
    }

    ans =
        readfile(bot_config_path(nullptr, "features/cat/user_list.json"), "[]");
    Json::Value user_list = string_to_json(ans);
    auto sz = user_list.size();
    for (Json::ArrayIndex i = 0; i < sz; ++i) {
        cat_map[user_list[i].asUInt64()] =
            Cat((userid_t)user_list[i].asUInt64());
    }
}

void catmain::save_map()
{
    Json::Value J;
    for (auto it : cat_map) {
        J.append(it.first);
    }
    writefile(bot_config_path(nullptr, "features/cat/user_list.json"),
              J.toStyledString());
}

Json::Value catmain::get_text() { return cat_text; }

void catmain::process(std::string message, const msg_meta &conf)
{
    std::string body;
    if (!cmd_strip_prefix(message, "*cat", body)) {
        return;
    }
    message = "&#91;cat&#93;" + body;

    if (message == "&#91;cat&#93;.help") {
        conf.p->cq_send("An interactive cat!\n"
                        "First use adopt to have one.\n"
                        "Then you can play, feed and so on!(start with *cat)",
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
                            "Use *cat.adopt name to get a cute cat!",
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
    (void)conf;
    return cmd_match_prefix(message, {"*cat"});
}
std::string catmain::help() { return "online cat. *cat.help"; }

DECLARE_FACTORY_FUNCTIONS(catmain)