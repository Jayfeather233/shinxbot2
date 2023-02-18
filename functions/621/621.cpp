#include "621.h"
#include "processable.h"
#include "utils.h"
#include <base64.hpp>

#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>

void parse_ja_to_map(const Json::Value &J, std::map<int64_t, bool>&mp){
    Json::Value::ArrayIndex sz = J.size();
    for(Json::Value::ArrayIndex i = 0; i < sz; i++){
        mp[J[i].asInt64()] = true;
    }
}
Json::Value parse_map_to_ja(const std::map<int64_t, bool>&mp){
    Json::Value Ja;
    for(auto u : mp){
        if(u.second){
            Ja.append(u.first);
        }
    }
    return Ja;
}

e621::e621(){
    std::ifstream afile;
    afile.open("./config/621_level.json", std::ios::in);

    if(afile.is_open()){
        std::string ans, line;
        while (!afile.eof()) {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();

        Json::Value J = string_to_json(ans);

        username = J["username"].asString();
        authorkey = J["authorkey"].asString();
        parse_ja_to_map(J["group"], group);
        parse_ja_to_map(J["user"], user);
        parse_ja_to_map(J["admin"], admin);
        parse_ja_to_map(J["not_search"], n_search);
    } else {
        
    }
}

std::string e621::deal_input(std::string input, bool is_pool){
    std::string res = my_replace(input, ' ', '+');
    
    if(res.find("score:") == res.npos && res.find("favcount:") == res.npos && !is_pool){
        res += "+score:>200+favcount:>400";
    }
    if(res.find("order:") == res.npos && !is_pool){
        res += "+order:random";
    }
    for(auto it : n_search){
        if(res.find(it.first) == res.npos){
            res += "+-" + it.first;
        }
    }
}

void e621::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    message = trim(message);
    if(message.find("621.set") == 0) {
        admin_set(trim(message.substr(7)), user_id, group_id);
        return;
    }
    if(message.find("621.del") == 0) {
        admin_del(trim(message.substr(7)), user_id, group_id);
        return;
    }

    if(message_type == "group"){
        if(!group[group_id]){
            setlog(LOG::WARNING, "621 in group " + std::to_string(group_id) + " but no permission");
            return;
        }
    } else {
        if(!user[user_id]){
            setlog(LOG::WARNING, "621 at user " + std::to_string(user_id) + " but no permission");
            return;
        }
    }

    if(message == "621.default") {
        std::string res = "\
如未指定favcount或score，默认加上favcount:>400 score:>200\n\
如未指定以下tags，默认不搜索";
        for(auto it : n_search){
            res += it.first + ",";
        }
        cq_send(res, message_type, user_id, group_id);
    }
    if(message.find("621.autocomplete") == 0){
        message = trim(message.substr(16));
        try{
            Json::Value Ja = string_to_json(
                do_get("https://e621.net",
                    "tags/autocomplete.json?search[name_matches]=" + message + "&expiry=7",
                    {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                    {"Authorization", "basic " + base64::to_base64(username + ":" + authorkey)}}));
            std::string res;
            Json::Value::ArrayIndex sz = Ja.size();
            for(Json::Value::ArrayIndex i = 0; i < sz; i++){
                res += Ja[i]["name"].asString() + "    " + std::to_string(Ja[i]["post_count"].asInt64()) + "\n";
            }
            cq_send(res, message_type, user_id, group_id);
        } catch (...) {
            cq_send("621 connect error. Try again.", message_type, user_id, group_id);
        }
        return;
    }

    bool is_pool = (message.find("pool:") != message.npos);
    std::string input = message.substr(3);
    bool get_tag = false;
    if(input.find(".tag") == 0){
        get_tag = true;
        input = trim(input.substr(4));
    }
    
    if(input.find("621.input") == 0){
        cq_send(deal_input(input, is_pool), message_type, user_id, group_id);
        return;
    }
    input = deal_input(input, is_pool);

    Json::Value J;
    int i;
    for(i = 0; i < 3; i++){
        try{
            J = string_to_json(
                do_get("https://e621.net", "posts.json?limit=50&tags=" + input,
                        {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                        {"Authorization", "basic " + base64::to_base64(username + ":" + authorkey)}})
            );
            break;
        } catch (...){

        }
    }
    if(i == 3){
        cq_send("Unable to connect to e621", message_type, user_id, group_id);
        setlog(LOG::WARNING, "621 at group " + std::to_string(group_id) + " by " + std::to_string(user_id) + " but unable to connect.");
        return;
    }

    int count = J["posts"].size();
    if(count == 0){
        cq_send("No image found.", message_type, user_id, group_id);
        setlog(LOG::WARNING, "621 at group " + std::to_string(group_id) + " by " + std::to_string(user_id) + " but no image found.");
        return;
    }

    if(!is_pool){
        Json::Value J2;
        J = J["posts"][0];
        int i;
        for(i=0;i<3;i++){
            if(get_tag){
                J2 = string_to_json(cq_send(get_image_tags(J), message_type,user_id, group_id));
            } else {
                J2 = string_to_json(cq_send(get_image_info(J,i), message_type,user_id, group_id));
            }
            if(J2["status"].asString() != "failed"){
                break;
            }
        }
        if(i == 3){
            cq_send("cannot send image due to tencent", message_type, user_id, group_id);
            setlog(LOG::WARNING, "621 at group " + std::to_string(group_id) + " by " + std::to_string(user_id) + " send failed.");
        } else {
            setlog(LOG::WARNING, "621 at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
        }
    } else {

    }
}

void e621::admin_set(std::string input, int64_t user_id, int64_t group_id){}
void e621::admin_del(std::string input, int64_t user_id, int64_t group_id){}
std::string e621::get_image_tags(Json::Value J){}
std::string e621::get_image_info(Json::Value J, int retry){}

bool e621::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){

}
std::string e621::help(){

}