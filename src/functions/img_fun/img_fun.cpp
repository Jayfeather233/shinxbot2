#include "img_fun.h"
#include "processable.h"
#include "utils.h"

#include <iostream>

std::string imgfun_help_msg = "图片处理。\n对称 axis=[0|1] order=[0|1] @或图片。\n\t axis， order为可选，axis指定x/y轴，order指定翻转哪边";

void img_fun::process(std::string message, const msg_meta &conf)
{
    if(message == "img_fun.help"){
        conf.p->cq_send(imgfun_help_msg, conf);
        return;
    }
    // TODO: mirror [axis]
    //      Kaleido_scope WanHuaTong
    //      rotate
    std::wstring wmessage = string_to_wstring(message);
    if (wmessage.find(L"对称") == 0) {
        char axis = 1;
        char order = 0;
        wmessage = trim(wmessage.substr(2));
        if (wmessage.find(L"axis=") == 0) {
            wmessage = trim(wmessage.substr(5));
            axis = wmessage[0] == L'1';
            wmessage = trim(wmessage.substr(1));
        }
        if (wmessage.find(L"order=") == 0) {
            wmessage = trim(wmessage.substr(6));
            order = wmessage[0] == L'1';
            wmessage = trim(wmessage.substr(1));
        }
        std::string fileurl;
        std::string filename;
        if (wmessage.find(L"[CQ:at") == 0) {
            int64_t userid = get_userid(wmessage);
            fileurl = "http://q1.qlogo.cn/g?b=qq&nk=" + std::to_string(userid) +
                      "&s=160";
            filename = "qq" + std::to_string(userid);
        }
        else if (wmessage.find(L"[CQ:img") == 0) {
            size_t index = wmessage.find(L",file=");
            index += 6;
            for (int i = index; i < wmessage.length(); i++) {
                if (wmessage[i] == L'.') {
                    filename =
                        wstring_to_string(wmessage.substr(index, i - index));
                    break;
                }
            }

            index = wmessage.find(L",url=");
            wmessage = wmessage.substr(index + 5);
            for (int i = 0; i < wmessage.length(); i++) {
                if (wmessage[i] == L']' || wmessage[i] == L',') {
                    fileurl = wstring_to_string(wmessage.substr(0, i));
                    break;
                }
            }
        }
        else {
            conf.p->cq_send("请将图片一并发出。", conf);
            return;
        }
        download(fileurl, "./resource/download/", filename);
        Magick::Image img;
        img.read("./resource/download/" + filename);
        if(img.animationDelay()){
            std::vector<Magick::Image> img_list;
            Magick::readImages(&img_list, "./resource/download/" + filename);
            mirrorImage(img_list, axis, order);
            Magick::writeImages(img_list.begin(), img_list.end(), "./resource/download/" + filename);
        } else {
            mirrorImage(img, axis, order);
            img.write("./resource/download/"+filename);
        }
        conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                            "/resource/download/" + filename + ",id=40000]",
                        conf);
    }
}
bool img_fun::check(std::string message, const msg_meta &conf)
{
    return (string_to_wstring(message).find(L"对称") == 0) || message == "img_fun.help";
}
std::string img_fun::help() {
    return "图片处理。 img_fun.help 获得更多帮助。";
}
