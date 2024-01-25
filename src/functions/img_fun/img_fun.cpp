#include "img_fun.h"
#include "processable.h"
#include "utils.h"

#include <iostream>

void img_fun::process(std::string message, const msg_meta &conf)
{
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
        else if (wmessage.find(L"[CQ:img")) {
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
            cq_send(conf.p, "请将图片一并发出。", conf);
            return;
        }
        download(fileurl, "./resource/download/", filename);
        Magick::Image img;
        img.read("./resource/download/" + filename);
        if(img.animationDelay()){
            std::vector<Magick::Image> img_list;
            Magick::readImages(&img_list, "./resource/download/" + filename);
            mirrorImage(img, axis, order);
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
    return (string_to_wstring(message).find(L"对称") == 0);
}
std::string img_fun::help() {}
