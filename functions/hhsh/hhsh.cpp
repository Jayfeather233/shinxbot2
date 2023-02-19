#include "hhsh.h"
#include "utils.h"

#include <iostream>
#include <jsoncpp/json/json.h>

void hhsh::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){

    if(message.length()<=5){
        cq_send("首字母缩写识别： hhsh + 缩写 ", message_type, user_id, group_id);
        return;
    }

    Json::Value J;
    J["text"] = my_replace(message.substr(4), ' ', ',');

    Json::Value Ja;

    try{
        Ja = string_to_json(do_post("https://lab.magiconch.com","api/nbnhhsh/guess", J));
    } catch (...) {
        setlog(LOG::WARNING, "failed to connect to hhsh");
        cq_send("failed to connect to hhsh", message_type, user_id, group_id);
        return;
    }

    std::string res;
    bool flg = false;

    Json::ArrayIndex sz = Ja.size();
    for(Json::ArrayIndex i = 0; i < sz; i++){
        J = Ja[i];
        if(flg) res += '\n';
        flg = true;
        if(J.isMember("inputting")){
            if(J["inputting"].size() == 0){
                res.append(J["name"].asString()).append("尚未录入");
            } else {
                res.append(J["name"].asString()).append("有可能是：\n");
                int t = 0;
                Json::Value maybe_value = J["inputting"];
                Json::ArrayIndex msz = maybe_value.size();
                for(Json::ArrayIndex i = 0; i < msz; i++){
                    res.append(maybe_value[i].asString());
                    res += ' ';
                    t++;
                    //if(t >= 10) break;
                }
            }
        } else if (J.isMember("trans")){
            if(J["trans"].isNull()){
                res.append(J["name"].asString()).append("未收录");
            } else {
                res.append(J["name"].asString()).append("是：\n");
                int t = 0;
                Json::Value maybe_value = J["trans"];
                Json::ArrayIndex msz = maybe_value.size();
                for(Json::ArrayIndex i = 0; i < msz; i++){
                    res.append(maybe_value[i].asString());
                    res += ' ';
                    t++;
                    //if(t >= 10) break;
                }
            }
        } else {
            res.append(J["name"].asString()).append("未收录");
        }
    }
    setlog(LOG::INFO, "nbnhhsh at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
    cq_send(res, message_type, user_id, group_id);
}
bool hhsh::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find("hhsh")==0;
}
std::string hhsh::help(){
    return "首字母缩写识别： hhsh+缩写";
}