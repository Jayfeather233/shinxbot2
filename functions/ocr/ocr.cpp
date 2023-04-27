#include "ocr.h"
#include "utils.h"

#include <iostream>
#include <map>
std::map<int64_t, bool> in_queue;

void ocr::process(std::string message, const msg_meta &conf)
{
    size_t index = message.find("[CQ:image,file=");
    if (index == std::string::npos) {
        cq_send("图来！", conf);
        in_queue[conf.user_id] = true;
        return;
    }
    in_queue[conf.user_id] = false;
    index += 15;
    size_t index2 = index;
    while (message[index2] != ',') {
        ++index2;
    }
    Json::Value J;
    J["image"] = message.substr(index, index2 - index);
    J = string_to_json(cq_send("ocr_image", J))["data"]["texts"];
    Json::ArrayIndex sz = J.size();
    std::string res;
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        res += J[i]["text"].asString() + " ";
    }
    cq_send(res, conf);
    setlog(LOG::INFO, "OCR at group " + std::to_string(conf.group_id) + " by " +
                          std::to_string(conf.user_id));
}
bool ocr::check(std::string message, const msg_meta &conf)
{
    return message.find(".ocr") == 0 || message.find(".OCR") == 0 ||
           (in_queue.find(conf.user_id) != in_queue.end() &&
            in_queue[conf.user_id] == true);
}
std::string ocr::help() { return "图片ocr。 .ocr 图片"; }
