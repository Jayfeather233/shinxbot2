#include "processable.h"
#include "utils.h"
#include "Anime_Img.h"

#include <iostream>

AnimeImg::AnimeImg(){}
void AnimeImg::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    try {
        Json::Value J = string_to_json(do_get("https://www.dmoe.cc/random.php?return=json"));
        setlog(LOG::INFO, "Auto2DAnimateImg at group " + std::to_string(group_id) + " by " + std::to_string(user_id));
        cq_send("[CQ:image,file=" + J["imgurl"].asString() + ",id=40000]", message_type, user_id, group_id);
        //Main.setNextLog("Auto2DAnimateImg at group " + group_id + " by "+user_id, 0);
    } catch (...) {
        cq_send("dmoe网站链接有问题", message_type, user_id, group_id);
        setlog(LOG::WARNING, "Auto2DAnimateImg at group " + std::to_string(group_id) + " Connect error");
    //    Main.setNextSender(message_type, user_id, group_id, "网站链接超时");
    //    Main.setNextLog("Auto2DAnimateImg at group " + group_id + " by "+user_id + " Connected Time Out",2);
    }
}
bool AnimeImg::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message=="来点二次元";
}
std::string AnimeImg::help(){
    return "纸片人图片：来点二次元";
}