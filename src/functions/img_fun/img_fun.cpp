#include "img_fun.h"
#include "image_utils.h"
#include "processable.h"
#include "utils.h"

#include <iostream>

static std::string imgfun_help_msg =
    "图片处理。\n对称 axis=[0|1] order=[0|1] @或图片。\n\t axis， "
    "order为可选，axis指定x/y轴，order指定翻转哪边\n旋转 fps=[24] order=[0|1] "
    "@或图片。\n万花筒 layers=[3] num=[8] @或图片。";

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
    std::map<uint64_t, img_fun_type>::iterator it;
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
        proc_type = (img_fun_type){img_fun_type::MIRROR, axis, order};
    }
    else if (wmessage.find(L"旋转") == 0) {
        int fps = 24;
        char order = 0;
        wmessage = trim(wmessage.substr(2));
        if (wmessage.find(L"fps=") == 0) {
            wmessage = trim(wmessage.substr(4));
            fps = std::max(0, std::min(50, (int)my_string2int64(wmessage)));
            wmessage = trim(wmessage.substr(wmessage.find(L' ') + 1));
        }
        if (wmessage.find(L"order=") == 0) {
            wmessage = trim(wmessage.substr(6));
            order = wmessage[0] == L'1';
            wmessage = trim(wmessage.substr(1));
        }
        proc_type = (img_fun_type){img_fun_type::ROTATE, fps, order};
    }
    else if (wmessage.find(L"万花筒") == 0) {
        int layers = 3;
        int nums = 8;
        wmessage = trim(wmessage.substr(3));
        if (wmessage.find(L"layer=") == 0) {
            wmessage = trim(wmessage.substr(6));
            layers = std::max(1, std::min(4, (int)my_string2int64(wmessage)));
            wmessage = trim(wmessage.substr(wmessage.find(L' ') + 1));
            printf("layers=%d\n", layers);
        }
        if (wmessage.find(L"num=") == 0) {
            wmessage = trim(wmessage.substr(4));
            nums = std::max(2, std::min(8, (int)my_string2int64(wmessage)));
            wmessage = trim(wmessage.substr(wmessage.find(L' ') + 1));
            printf("nums=%d\n", nums);
        }
        proc_type = (img_fun_type){img_fun_type::KALEIDO, layers, nums};
    }
    else if ((it = is_input.find(conf.user_id)) != is_input.end()) {
        proc_type = is_input[conf.user_id];
    }
    else {
        // ?
        return;
    }

    if (wmessage.find(L"[CQ:at") != wmessage.npos) {
        uint64_t userid = my_string2uint64(wmessage);
        fileurl =
            "http://q1.qlogo.cn/g?b=qq&nk=" + std::to_string(userid) + "&s=160";
        filename = "qq" + std::to_string(userid);
    }
    else if (wmessage.find(L"[CQ:image") != wmessage.npos) {
        std::time_t nt = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        tm tt = *localtime(&nt);

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << tt.tm_hour << "_"
            << std::setw(2) << std::setfill('0') << tt.tm_min << "_"
            << std::setw(2) << std::setfill('0') << tt.tm_sec;
        filename = std::to_string(conf.user_id) + "_" + oss.str();

        size_t index = wmessage.find(L",url=");
        wmessage = wmessage.substr(index + 5);
        for (size_t i = 0; i < wmessage.length(); i++) {
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
    download(cq_decode(fileurl), "./resource/download/", filename);
    Magick::Image img;
    bool mgif = false;
    try {
        img.read("./resource/download/" + filename);
    }
    catch (...) {
        mgif = true;
    }
    if (img.animationDelay() || proc_type.type == img_fun_type::ROTATE ||
        mgif) {
        std::vector<Magick::Image> img_list;
        Magick::readImages(&img_list, "./resource/download/" + filename);
        if (proc_type.type == img_fun_type::MIRROR) {
            filename += "_mir.gif";
            mirrorImage(img_list, proc_type.para1, proc_type.para2);
        }
        else if (proc_type.type == img_fun_type::ROTATE) {
            filename += "_rot.gif";
            img_list = rotateImage(img, proc_type.para1, proc_type.para2);
        }
        else if (proc_type.type == img_fun_type::KALEIDO) {
            filename += "_kal.gif";
            kaleido(img_list, proc_type.para1, proc_type.para2);
        }
        Magick::writeImages(img_list.begin(), img_list.end(),
                            "./resource/download/" + filename);
    }
    else {
        if (proc_type.type == img_fun_type::MIRROR) {
            filename += "_mir.png";
            mirrorImage(img, proc_type.para1, proc_type.para2);
        }
        else if (proc_type.type == img_fun_type::KALEIDO) {
            filename += "_kal.png";
            kaleido(img, proc_type.para1, proc_type.para2);
        }
        img.write("./resource/download/" + filename);
    }
    conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                        "/resource/download/" + filename + ",id=40000]",
                    conf);
    return;
}
bool img_fun::check(std::string message, const msg_meta &conf)
{
    std::wstring wmes = string_to_wstring(message);
    return message == "img_fun.help" || wmes.find(L"对称 ") == 0 ||
           wmes == L"对称" || wmes.find(L"旋转 ") == 0 || wmes == L"旋转" ||
           wmes.find(L"万花筒 ") == 0 || wmes == L"万花筒" ||
           is_input.find(conf.user_id) != is_input.end();
}
std::string img_fun::help() { return "图片处理。 img_fun.help 获得更多帮助。"; }

extern "C" processable* create() {
    return new img_fun();
}