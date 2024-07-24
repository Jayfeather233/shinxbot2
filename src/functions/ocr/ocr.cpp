#include "ocr.h"
#include "utils.h"

#include <iostream>
#include <map>
static std::map<userid_t, bool> in_queue;

std::string ocr::ocr_tostring(const Json::Value &J) {
    Json::ArrayIndex sz = J.size();
    std::ostringstream oss;
    long long top, bottom, center;
    long long new_top, new_bottom;
    for(Json::ArrayIndex i = 0;i<sz;i++){
        top = J[i]["coordinates"][0]["y"].asInt64();
        bottom = J[i]["coordinates"][2]["y"].asInt64();
        center = (top+bottom)>>1;
        oss << J[i]["text"].asString();
        ++i;

        for(;i<sz;i++){
            new_top = J[i]["coordinates"][0]["y"].asInt64();
            new_bottom = J[i]["coordinates"][2]["y"].asInt64();
            if(new_top > center || new_bottom < center) {
                oss << std::endl;
                --i;
                break;
            } else {
                top = (top + new_top) >> 1;
                bottom = (bottom + new_bottom) >> 1;
                oss << ' ' << J[i]["text"].asString();
            }
        }
    }
    oss << std::endl;
    return oss.str();
}

void ocr::process(std::string message, const msg_meta &conf)
{
    size_t index = message.find("[CQ:image,file=");
    if (index == std::string::npos) {
        conf.p->cq_send("图来！", conf);
        in_queue[conf.user_id] = true;
        return;
    }
    in_queue[conf.user_id] = false;
    index = message.find(",url=", index);
    index += 5;
    size_t index2 = index;
    while (message[index2] != ',' && message[index2] != ']') {
        ++index2;
    }
    Json::Value J;
    J["image"] = cq_decode(message.substr(index, index2 - index));
    J = string_to_json(conf.p->cq_send("ocr_image", J));
    if(J["data"].isNull()) return;
    J = J["data"]["texts"];
    std::string res = ocr_tostring(J);
    conf.p->cq_send(res, conf);
    conf.p->setlog(LOG::INFO, "OCR at group " + std::to_string(conf.group_id) +
                                  " by " + std::to_string(conf.user_id));
}
bool ocr::check(std::string message, const msg_meta &conf)
{
    return message.find(".ocr") == 0 || message.find(".OCR") == 0 ||
           (in_queue.find(conf.user_id) != in_queue.end() &&
            in_queue[conf.user_id] == true);
}
std::string ocr::help() { return "图片ocr。 .ocr 图片"; }

extern "C" processable* create() {
    return new ocr();
}