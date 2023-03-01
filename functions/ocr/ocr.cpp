#include "ocr.h"
#include "utils.h"

#include <iostream>
#include <map>
std::map<int64_t, bool> in_queue;

void ocr::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    int index = message.find("[CQ:image,file=");
    if(index == std::string::npos){
        cq_send("图来！", message_type, user_id, group_id);
        in_queue[user_id] = true;
        return;
    }
    in_queue[user_id] = false;
    index += 15;
    int index2 = index;
    while(message[index2]!=','){
        ++index2;
    }
    Json::Value J;
    J["image"] = message.substr(index, index2-index);
    J = string_to_json(
        cq_send("ocr_image", J)
    )["data"]["texts"];
    Json::ArrayIndex sz = J.size();
    std::string res;
    for(Json::ArrayIndex i = 0; i < sz; i++){
        res+=J[i]["text"].asString() + " ";
    }
    cq_send(res, message_type, user_id, group_id);
    setlog(LOG::INFO,"OCR at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
}
bool ocr::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find(".ocr") == 0 || message.find(".OCR") == 0 || (in_queue.find(user_id)!=in_queue.end() && in_queue[user_id] == true);
}
std::string ocr::help(){
    return "图片ocr。 .ocr 图片";
}