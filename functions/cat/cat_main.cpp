#include "cat.h"

#include <fstream>
#include <filesystem>

Json::Value catmain::cat_text;

catmain::catmain(){
    std::ifstream afile;
    afile.open("./config/cat.json", std::ios::in);

    if (afile.is_open()){
        std::string ans, line;
        while (!afile.eof())
        {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();

        Json::Value J = string_to_json(ans);

        cat_text = J;

        afile.close();
    } else {
        setlog(LOG::ERROR, "Missing file: ./config/cat.json");
    }

    afile.open("./config/cats/user_list.json", std::ios::in);
    if (afile.is_open()){
        std::string ans, line;
        while (!afile.eof())
        {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();

        Json::Value user_list = string_to_json(ans);

        auto sz = user_list.size();

        for(Json::ArrayIndex i = 0; i < sz; ++i){
            cat_map[user_list[i].asInt64()] = Cat((int64_t)user_list[i].asInt64());
        }

        afile.close();
    } else {
        if(!std::filesystem::exists("./config/cats"))
            std::filesystem::create_directory("./config/cats");
        std::ofstream of("./config/cats/user_list.json", std::ios::out);
        of << "[]";
        of.close();
    }
}

void catmain::save_map(){
    Json::Value J;
    for(auto it : cat_map){
        J.append(it.first);
    }
    std::ofstream of("./config/cats/user_list.json", std::ios::out);
    of << J.toStyledString();
    of.close();
}

Json::Value catmain::get_text(){
    return cat_text;
}

void catmain::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(message == "[cat].help"){
        cq_send("\
An interactive cat!\n\
First use adopt to have one.\n\
Then you can play, feed and so on!(start with[cat])", message_type, user_id, group_id);
        return;
    }
    if(message.find("adopt") != std::string::npos){
        auto it = cat_map.find(user_id);
        if(it != cat_map.end()){
            cq_send("You already have one!", message_type, user_id, group_id);
            return;
        } else {
            std::string name = trim(message.substr(11));
            if(name.length() <= 3){
                cq_send("Please give it a name", message_type, user_id, group_id);
                return;
            } else {
                Cat newcat(name, user_id);
                cq_send(newcat.adopt(), message_type, user_id, group_id);
            }
        }
    } else {
        auto it = cat_map.find(user_id);
        if(it == cat_map.end()){
            cq_send("You don't have one!\nUse [cat].adopt name to get a cute cat!", message_type, user_id, group_id);
            return;
        } else {
            cq_send(cat_map[user_id].process(message), message_type, user_id, group_id);
        }
    }
}
bool catmain::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find("[cat]") == 0;
}
std::string catmain::help(){
    return "online cat. [cat].help";
}