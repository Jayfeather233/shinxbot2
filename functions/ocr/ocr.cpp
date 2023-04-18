#include "ocr.h"
#include "utils.h"

#include <iostream>
#include <map>
std::map<int64_t, bool> in_queue;

void ocr::process(shinx_message msg){
    size_t index = msg.message.find("[CQ:image,file=");
    if(index == std::string::npos){
        msg.message = "图来！";
        cq_send(msg);
        in_queue[msg.user_id] = true;
        return;
    }
    in_queue[msg.user_id] = false;
    index += 15;
    size_t index2 = index;
    while(msg.message[index2]!=','){
        ++index2;
    }
    Json::Value J;
    J["image"] = msg.message.substr(index, index2-index);
    J = string_to_json(
        cq_send("ocr_image", J)
    )["data"]["texts"];
    Json::ArrayIndex sz = J.size();
    std::string res;
    for(Json::ArrayIndex i = 0; i < sz; i++){
        res+=J[i]["text"].asString() + " ";
    }
    msg.message = res;
    cq_send(msg);
    setlog(LOG::INFO,"OCR at group " + std::to_string(msg.group_id) + " by " + std::to_string(msg.user_id));
}
bool ocr::check(shinx_message msg){
    return msg.message.find(".ocr") == 0 || msg.message.find(".OCR") == 0 || (in_queue.find(msg.user_id)!=in_queue.end() && in_queue[msg.user_id] == true);
}
std::string ocr::help(){
    return "图片ocr。 .ocr 图片";
}