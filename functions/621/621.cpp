#include "621.h"
#include "processable.h"
#include "utils.h"

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

    } else {
        
    }
}
void e621::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){

}
bool e621::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){

}
std::string e621::help(){

}