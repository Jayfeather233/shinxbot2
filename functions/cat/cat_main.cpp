#include "cat.h"

#include <iostream>
#include <fstream>
#include <filesystem>

Json::Value catmain::cat_text;

catmain::catmain(){
    std::string ans = readfile("./config/cat.json");
    if (ans != ""){
        Json::Value J = string_to_json(ans);
        cat_text = J;
    } else {
        setlog(LOG::ERROR, "Missing file: ./config/cat.json");
    }

    ans = readfile("./config/cats/user_list.json", "[]");
    Json::Value user_list = string_to_json(ans);
    auto sz = user_list.size();
    for(Json::ArrayIndex i = 0; i < sz; ++i){
        cat_map[user_list[i].asInt64()] = Cat((int64_t)user_list[i].asInt64());
    }
}

void catmain::save_map(){
    Json::Value J;
    for(auto it : cat_map){
        J.append(it.first);
    }
    writefile("./config/cats/user_list.json", J.toStyledString());
}

Json::Value catmain::get_text(){
    return cat_text;
}

void catmain::process(shinx_message msg){
    if(msg.message == "&#91;cat&#93;.help"){
        msg.message = "An interactive cat!\n"
                      "First use adopt to have one.\n"
                      "Then you can play, feed and so on!(start with[cat])";
        cq_send(msg);
        return;
    }
    if(msg.message.find(".adopt") == 13){
        auto it = cat_map.find(msg.user_id);
        if(it != cat_map.end()){
            msg.message = "You already have one!";
            cq_send(msg);
            return;
        } else {
            std::string name = trim(msg.message.substr(19));
            if(name.length() <= 0){
                msg.message = "Please give it a name";
                cq_send(msg);
                return;
            } else {
                Cat newcat(name, msg.user_id);
                cat_map[msg.user_id] = newcat;
                msg.message = newcat.adopt();
                cq_send(msg);
                save_map();
            }
        }
    } else if(msg.message.find(".rename") == 13){
        auto it = cat_map.find(msg.user_id);
        if(it == cat_map.end()){
            msg.message = "You don't have one!";
            cq_send(msg);
            return;
        } else {
            std::string name = trim(msg.message.substr(20));
            if(name.length() <= 0){
                msg.message = "Please give it a name";
                cq_send(msg);
                return;
            } else {
                msg.message = it->second.rename(name);
                cq_send(msg);
                save_map();
            }
        }
    } else {
        auto it = cat_map.find(msg.user_id);
        if(it == cat_map.end()){
            msg.message = "You don't have one!\nUse [cat].adopt name to get a cute cat!";
            cq_send(msg);
            return;
        } else {
            msg.message = it->second.process(msg.message);
            cq_send(msg);
        }
    }
}
bool catmain::check(shinx_message msg){
    return msg.message.find("&#91;cat&#93;") == 0;
}
std::string catmain::help(){
    return "online cat. [cat].help";
}