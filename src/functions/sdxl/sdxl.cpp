#include "sdxl.h"

#include <jsoncpp/json/json.h>

std::string help_msg =
    "SDXL:\n"
    ".sdxl <prompt> for generate image\n<prompt> can be two part, "
    "<positive_prompt> [neg: <negative_prompt>]";

void sdxl::process(std::string message, const msg_meta &conf)
{
    if(message.find(".sdxl.help") == 0){
        cq_send(conf.p, help_msg, conf);
        return;
    }
    Json::Value J;
    size_t inx = message.find("neg:");
    if (inx == message.npos) {
        J["prompt"] = message.substr(5);
        J["negative_prompt"] =
            "(worst quality:2), (low quality:2), (normal quality:2), lowres, "
            "normal quality, ((grayscale)), skin spots, acnes, skin blemishes, "
            "age spot, (ugly:1.331), (duplicate:1.331), (morbid:1.21), "
            "(mutilated:1.21), (tranny:1.331), mutated hands, (poorly drawn "
            "hands:1.5), blurry, (bad anatomy:1.21), (bad proportions:1.331), "
            "extra limbs, (disfigured:1.331), (missing arms:1.331), (extra "
            "legs:1.331), (fused fingers:1.61051), (too many fingers:1.61051), "
            "(unclear eyes:1.331), lowers, bad hands, missing fingers, extra "
            "digit,bad hands, missing fingers, (((extra arms and legs)))";
    }
    else {
        J["prompt"] = message.substr(5, inx - 5);
        J["negative_prompt"] =
            message.substr(inx + 4) +
            " (worst quality:2), (low quality:2), (normal quality:2), lowres, "
            "normal quality, ((grayscale)), skin spots, acnes, skin blemishes, "
            "age spot, (ugly:1.331), (duplicate:1.331), (morbid:1.21), "
            "(mutilated:1.21), (tranny:1.331), mutated hands, (poorly drawn "
            "hands:1.5), blurry, (bad anatomy:1.21), (bad proportions:1.331), "
            "extra limbs, (disfigured:1.331), (missing arms:1.331), (extra "
            "legs:1.331), (fused fingers:1.61051), (too many fingers:1.61051), "
            "(unclear eyes:1.331), lowers, bad hands, missing fingers, extra "
            "digit,bad hands, missing fingers, (((extra arms and legs)))";
    }
    J["source"] = "sdxlturbo.ai";

    J = string_to_json(
        do_post("https://sd.cuilutech.com/sdapi/turbo/txt2img", J, {}, true));
    if (J["code"].asInt() != 200 || J["msg"].asString() != "success") {
        cq_send(conf.p, J.toStyledString(), conf);
    }
    else {
        cq_send(conf.p,
                "[CQ:image,file=" + J["data"]["image_url"].asString() +
                    ",id=40000]",
                conf);
    }
}
bool sdxl::check(std::string message, const msg_meta &conf)
{
    return message.find(".sdxl") == 0;
}
std::string sdxl::help()
{
    return "StableDiffusion XL-Turbo. .sdxl.help for more.";
}