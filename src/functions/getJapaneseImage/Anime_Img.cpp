#include "Anime_Img.h"
#include "processable.h"
#include "utils.h"

#include <iostream>

void AnimeImg::process(std::string message, const msg_meta &conf)
{
    Json::Value J;
    J["message_id"] = conf.message_id;
    conf.p->cq_send("mark_msg_as_read", J);
    try {
        Json::Value J = string_to_json(
            do_get("https://www.dmoe.cc/random.php?return=json"));
        conf.p->setlog(LOG::INFO, "Auto2DAnimateImg at group " +
                                      std::to_string(conf.group_id) + " by " +
                                      std::to_string(conf.user_id));
        conf.p->cq_send(
            "[CQ:image,file=" + J["imgurl"].asString() + ",id=40000]", conf);
    }
    catch (...) {
        conf.p->cq_send("dmoe网站链接有问题", conf);
        conf.p->setlog(LOG::WARNING, "Auto2DAnimateImg at group " +
                                         std::to_string(conf.group_id) +
                                         " Connect error");
    }
}
bool AnimeImg::check(std::string message, const msg_meta &conf)
{
    return message == "来点二次元";
}
std::string AnimeImg::help() { return "纸片人图片：来点二次元"; }

extern "C" processable* create() {
    return new AnimeImg();
}