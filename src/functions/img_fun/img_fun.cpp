#include "img_fun.h"
#include "processable.h"
#include "utils.h"

#include <iostream>

static std::string imgfun_help_msg =
    "图片处理。\n对称 axis=[0|1] order=[0|1] @或图片。\n\t axis， "
    "order为可选，axis指定x/y轴，order指定翻转哪边";

void img_fun::process(std::string message, const msg_meta &conf)
{
    if (message == "img_fun.help") {
        conf.p->cq_send(imgfun_help_msg, conf);
        return;
    }
    // TODO: mirror [axis]
    //      Kaleido_scope WanHuaTong
    //      rotate
    std::wstring wmessage = string_to_wstring(message);
    std::string fileurl;
    std::string filename;
    img_fun_type proc_type;
    std::map<int64_t, img_fun_type>::iterator it;
    if (wmessage.find(L"对称 ") == 0) {
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
        proc_type = (img_fun_type){img_fun_type::MIRROR, axis, order};
    } else if(wmessage.find(L"旋转 ") == 0) {
        char order = 0;
        wmessage = trim(wmessage.substr(2));
        if (wmessage.find(L"order=") == 0) {
            wmessage = trim(wmessage.substr(6));
            order = wmessage[0] == L'1';
            wmessage = trim(wmessage.substr(1));
        }
        proc_type = (img_fun_type){img_fun_type::ROTATE, order};
    } else if((it = is_input.find(conf.user_id)) != is_input.end()){
        proc_type = is_input[conf.user_id];
    } else {
        // ?
        return;
    }
    
    if (wmessage.find(L"[CQ:at") != wmessage.npos) {
        int64_t userid = get_userid(wmessage);
        fileurl = "http://q1.qlogo.cn/g?b=qq&nk=" + std::to_string(userid) +
                    "&s=160";
        filename = "qq" + std::to_string(userid);
    }
    else if (wmessage.find(L"[CQ:image") != wmessage.npos) {
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
        conf.p->cq_send("图来", conf);
        is_input[conf.user_id] = proc_type;
        return;
    }

    is_input.erase(conf.user_id);
    download(fileurl, "./resource/download/", filename);
    Magick::Image img;
    img.read("./resource/download/" + filename);
    if (img.animationDelay()) {
        std::vector<Magick::Image> img_list;
        Magick::readImages(&img_list, "./resource/download/" + filename);
        if(proc_type.type == img_fun_type::MIRROR)
            mirrorImage(img_list, proc_type.para1, proc_type.para2);
        else if(proc_type.type == img_fun_type::ROTATE)
            conf.p->cq_send("No gif in rotate.", conf);
        else if(proc_type.type == img_fun_type::KALEIDO)
            kaleido(img_list);
        Magick::writeImages(img_list.begin(), img_list.end(),
                            "./resource/download/" + filename);
    }
    else {
        if(proc_type.type == img_fun_type::MIRROR)
            mirrorImage(img, proc_type.para1, proc_type.para2);
        else if(proc_type.type == img_fun_type::ROTATE)
            rotateImage(img, 25, proc_type.para1);
        else if(proc_type.type == img_fun_type::KALEIDO)
            kaleido(img);
        img.write("./resource/download/" + filename);
    }
    conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                        "/resource/download/" + filename + ",id=40000]",
                    conf);
    return;
}
bool img_fun::check(std::string message, const msg_meta &conf)
{
    return message == "img_fun.help" ||
           string_to_wstring(message).find(L"对称 ") == 0 ||
           string_to_wstring(message).find(L"旋转 ") == 0 ||
           string_to_wstring(message).find(L"万花筒 ") == 0 ||
           is_input.find(conf.user_id) != is_input.end();
}
std::string img_fun::help() { return "图片处理。 img_fun.help 获得更多帮助。"; }
