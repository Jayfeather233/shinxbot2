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

void catmain::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(message == "&#91;cat&#93;.help"){
        cq_send("\
An interactive cat!\n\
First use adopt to have one.\n\
Then you can play, feed and so on!(start with[cat])", message_type, user_id, group_id);
        return;
    }
    if(message.find(".adopt") == 13){
        auto it = cat_map.find(user_id);
        if(it != cat_map.end()){
            cq_send("You already have one!", message_type, user_id, group_id);
            return;
        } else {
            std::string name = trim(message.substr(19));
            if(name.length() <= 0){
                cq_send("Please give it a name", message_type, user_id, group_id);
                return;
            } else {
                Cat newcat(name, user_id);
                cat_map[user_id] = newcat;
                cq_send(newcat.adopt(), message_type, user_id, group_id);
                save_map();
            }
        }
    } else if(message.find(".rename") == 13){
        auto it = cat_map.find(user_id);
        if(it == cat_map.end()){
            cq_send("You don't have one!", message_type, user_id, group_id);
            return;
        } else {
            std::string name = trim(message.substr(20));
            if(name.length() <= 0){
                cq_send("Please give it a name", message_type, user_id, group_id);
                return;
            } else {
                cq_send(it->second.rename(name), message_type, user_id, group_id);
                save_map();
            }
        }
    } else {
        auto it = cat_map.find(user_id);
        if(it == cat_map.end()){
            cq_send("You don't have one!\nUse [cat].adopt name to get a cute cat!", message_type, user_id, group_id);
            return;
        } else {
            cq_send(it->second.process(message), message_type, user_id, group_id);
        }
    }
}
bool catmain::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find("&#91;cat&#93;") == 0;
}
std::string catmain::help(){
    return "online cat. [cat].help";
}