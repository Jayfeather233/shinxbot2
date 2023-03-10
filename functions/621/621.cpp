#include "621.h"
#include "processable.h"
#include "utils.h"
#include <base64.hpp>

#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>

e621::e621(){
    std::string ans = readfile("./config/621_level.json", "{}");
    Json::Value J = string_to_json(ans);

    username = J["username"].asString();
    authorkey = J["authorkey"].asString();
    parse_json_to_map(J["group"], group);
    parse_json_to_map(J["user"], user);
    parse_json_to_map(J["admin"], admin);
    Json::ArrayIndex sz = J["n_search"].size();
    for(Json::ArrayIndex i = 0; i < sz; i++){
        n_search.insert(J["n_search"][i].asString());
    }
}

std::string e621::deal_input(const std::string &input, bool is_pool){
    std::string res = my_replace(input, ' ', '+');
    
    if(res.find("score:") == res.npos && res.find("favcount:") == res.npos && !is_pool && res.find("id:") == res.npos){
        res += "+score:>200+favcount:>400";
    }
    if(res.find("order:") == res.npos && !is_pool){
        res += "+order:random";
    }
    //res += "+-animated";
    if(!is_pool)
        for(std::string it : n_search){
            if(res.find(it) == res.npos){
                res += "+-" + it;
            }
        }
    return res;
}

std::string number_trans(int64_t u){
    if (u > 1000000) return std::to_string(u / 1000000) + "M";
    else if (u > 1000) return std::to_string(u / 1000) + "k";
    else return std::to_string(u);
}

void e621::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    message = trim(message);
    if(message.find("621.add") == 0) {
        admin_set(trim(message.substr(7)), message_type, user_id, group_id, true);
        return;
    }
    if(message.find("621.del") == 0) {
        admin_set(trim(message.substr(7)), message_type, user_id, group_id, false);
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
????????????favcount???score???????????????favcount:>400 score:>200\n\
??????????????????tags??????????????????";
        for(std::string it : n_search){
            res += it + ",";
        }
        cq_send(res, message_type, user_id, group_id);
    }
    if(message.find("621.autocomplete") == 0){
        message = trim(message.substr(16));
        try{
            Json::Value Ja = string_to_json(
                do_get("https://e621.net/tags/autocomplete.json?search[name_matches]=" + message + "&expiry=7",
                    {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                    {"Authorization", "basic " + base64::to_base64(username + ":" + authorkey)}}, true));
            std::string res;
            Json::ArrayIndex sz = Ja.size();
            for(Json::ArrayIndex i = 0; i < sz; i++){
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
    if(input.find(".input") == 0){
        cq_send(deal_input(input.substr(6), is_pool), message_type, user_id, group_id);
        return;
    }
    if(input.length() <= 1){
        input += " fav:jayfeather233 eeveelution";
    }
    input = deal_input(input, is_pool);

    Json::Value J;
    int i;
    for(i = 0; i < 3; i++){
        try{
            J = string_to_json(
                do_get("https://e621.net/posts.json?limit=50&tags=" + input,
                        {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                        {"Authorization", "basic " + base64::to_base64(username + ":" + authorkey)}}, true)
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
                J2 = string_to_json(cq_send(get_image_tags(J) + (i ? "\ntx????????????????????????" : ""), message_type,user_id, group_id));
            } else {
                J2 = string_to_json(cq_send(get_image_info(J, count, is_pool, i) + (i ? "\ntx????????????????????????" : ""), message_type,user_id, group_id));
            }
            if(J2["status"].asString() != "failed"){
                break;
            }
        }
        if(i == 3){
            cq_send("cannot send image due to Tencent", message_type, user_id, group_id);
            setlog(LOG::WARNING, "621 at group " + std::to_string(group_id) + " by " + std::to_string(user_id) + " send failed.");
        } else {
            setlog(LOG::INFO, "621 at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
        }
    } else {
        int64_t pool_id = J["posts"][0]["pools"][0].asInt64();
        
        Json::Value J3 = string_to_json(
            do_get("https://e621.net/pools.json?search[id]=" + std::to_string(pool_id),
                    {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                    {"Authorization", "basic " + base64::to_base64(username + ":" + authorkey)}}, true)
        )[0];

        std::string res_message;
        res_message = "??????\n";
        res_message += std::to_string(get_botqq()) + " " + J3["category"].asString() + ": " + J3["name"].asString() + "\n";
        res_message += std::to_string(get_botqq()) + " ?????????\n?????????" + J3["description"].asString() + "\n????????????\n";
        res_message += std::to_string(get_botqq()) + " ?????? " + std::to_string(J3["post_count"].asInt64()) + "???\n";
        J3 = J3["post_ids"];
        Json::ArrayIndex sz = J3.size();
        for(Json::ArrayIndex i = 0; i < sz; i++){
            for(Json::ArrayIndex j = 0; j < count; j++){
                if(J3[i].asInt64() == J["posts"][j]["id"].asInt64()){
                    res_message += std::to_string(get_botqq()) + " ?????????\n";
                    res_message += get_image_info(J["posts"][j], count, is_pool, 1);
                    res_message += "\n????????????\n";
                }
            }
        }

        Json::Value J_send;
        J_send["post_type"] = "message";
        J_send["message"] = res_message;
        J_send["message_type"] = message_type;
        J_send["message_id"] = -1;
        J_send["user_id"] = user_id;
        J_send["group_id"] = group_id;
        input_process(new std::string(J_send.toStyledString()));
        setlog(LOG::INFO, "621 pool at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
    }
}

void e621::admin_set(const std::string &input, const std::string message_type, int64_t user_id, int64_t group_id, bool flg){
    std::istringstream iss(input);
    auto it = admin.find(user_id);
    if(it == admin.end() || it->second == false){
        return;
    }
    std::string type;
    iss >> type;
    if(type == "this"){
        if(message_type == "group"){
            group[group_id] = flg;
        } else {
            user[user_id] = flg;
        }
    } else {
        int64_t id;
        iss >> id;
        if(type == "group"){
            group[id] = flg;
        } else if(type == "user"){
            user[id] = flg;
        } else {
            cq_send("621.set [this/group/user] [id (when not 'this')]", message_type, user_id, group_id);
        }
    }
    save();
    cq_send("set done.", message_type, user_id, group_id);
}
void e621::save(){
    Json::Value J;
    J["authorkey"] = authorkey;
    J["username"] = username;
    J["user"] = parse_map_to_json(user);
    J["group"] = parse_map_to_json(group);
    J["admin"] = parse_map_to_json(admin);
    Json::Value J2;
    for(std::string u : n_search){
        J2.append(u);
    }
    J["n_search"] = J2;

    writefile("./config/621_level.json", J.toStyledString());
}
std::string e621::get_image_tags(const Json::Value &J){
    std::string s;
    Json::ArrayIndex sz;
    Json::Value J2;
    J2 = J["tags"];
    s += "artist:";
    sz = J2["artist"].size();
    for(Json::ArrayIndex i = 0; i < sz; i++) s += J2["artist"][i].asString() + " ";
    s+="\n";
    s += "character:";
    sz = J2["character"].size();
    for(Json::ArrayIndex i = 0; i < sz; i++) s += J2["character"][i].asString() + " ";
    s+="\n";
    s += "species:";
    sz = J2["species"].size();
    for(Json::ArrayIndex i = 0; i < sz; i++) s += J2["species"][i].asString() + " ";
    s+="\n";
    return s;
}

std::string e621::get_image_info(const Json::Value &J, int count, bool poolFlag, int retry) {
    std::string imageUrl;
    if (J.isMember("file") && retry <= 0){
        imageUrl = J["file"]["url"].asString();
    } else if (J.isMember("sample") && retry <= 1) {
        imageUrl = J["sample"]["url"].asString();
    } else if (J.isMember("preview")) {
        imageUrl = J["preview"]["url"].asString();
    } else {
        throw "";
    }

    int64_t id = J["id"].asInt64();
    int64_t fav_count = J["fav_count"].asInt64();
    int64_t score = J["score"]["total"].asInt64();

    std::stringstream quest;
    if (!poolFlag && count != 50) {
        quest << "??????" << count << "?????????\n";
    }
    if (poolFlag && count == 50) {
        quest << "??????" << count << "?????????\n";
    }

    size_t extPos = 0, tmpPos;
    while ((tmpPos = imageUrl.find(".", extPos)) != std::string::npos) {
        extPos = tmpPos + 1;
    }
    std::string fileExt = imageUrl.substr(extPos);
    std::string imageLocalPath = std::to_string(id) + '.' + fileExt;

    if (!std::ifstream("./resource/download/e621/" + imageLocalPath) && fileExt != "webm" && fileExt != "mp4") {
        download(imageUrl, "./resource/download/e621", imageLocalPath, true);
    }
    if(fileExt != "gif" && fileExt != "webm" && fileExt != "mp4")
        addRandomNoise("./resource/download/e621/" + imageLocalPath);

    if(fileExt != "webm" && fileExt != "mp4"){
        quest << (fileExt == "gif" ? "Get gif:\n" : "") << "[CQ:image,file=file://" << get_local_path() << "/resource/download/e621/" << imageLocalPath << ",id=40000]\n";
    } else {
        quest << "Get video. id: " + std::to_string(id) << std::endl;
    }
    quest << "Fav_count: " << fav_count << "  Score: " << score << "\n";

    auto poolList = J["pools"];
    if (poolList.isArray() && poolList.size() > 0) {
        quest << "pools:";
        for (Json::ArrayIndex i = 0; i < poolList.size(); i++) {
            quest << " " << poolList[i].asInt();
        }
        quest << '\n';
    }
    quest << "id: " << id;
    return quest.str();
}


bool e621::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(!message.find("621") == 0) return false;
    if(message_type == "group"){
        auto it = group.find(group_id);
        if(it == group.end() || it->second == false){
            return false;
        }
    } else {
        auto it = user.find(user_id);
        if(it == user.end() || it->second == false){
            return false;
        }
    }
    return true;
}
std::string e621::help(){
    return "";
}