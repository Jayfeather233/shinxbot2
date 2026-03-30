#include "img_fun.h"
#include "image_utils.h"
#include "processable.h"
#include "utils.h"

#include <iostream>

static std::string imgfun_help_msg =
    "图片处理。\n对称 axis=[0|1] order=[0|1] @或图片。\n\t axis， "
    "order为可选，axis指定x/y轴，order指定翻转哪边\n旋转 fps=[24] order=[0|1] "
    "@或图片。\n万花筒 num=[3~12] @或图片。";

void img_fun::process(std::string message, const msg_meta &conf)
{
    BarInfo p(0, "图片处理初始化");
    conf.p->registerBar(&p);
    if (cmd_match_exact(message, {"img_fun.help"})) {
        conf.p->cq_send(imgfun_help_msg, conf);
        return;
    }
    // TODO: mirror [axis]
    //      Kaleido_scope WanHuaTong
    //      rotate
    std::wstring wmessage = trim(string_to_wstring(message));
    std::string fileurl;
    std::string filename;
    img_fun_type proc_type = (img_fun_type){img_fun_type::MIRROR, 1, 0};
    std::map<userid_t, img_fun_type>::iterator it = is_input.end();

    auto parse_axis_or_order = [](const std::wstring &token,
                                  const std::wstring &key,
                                  int &target) -> bool {
        std::wstring body;
        if (!cmd_strip_prefix(token, key, body) || body.empty()) {
            return false;
        }
        target = (body[0] == L'1') ? 1 : 0;
        return true;
    };

    auto parse_command = [&](const std::wstring &input, img_fun_type &out,
                             std::wstring &payload_out) -> bool {
        struct rule {
            std::wstring cmd;
            std::function<bool(const std::wstring &, img_fun_type &,
                               std::wstring &)>
                handler;
        };

        const std::vector<rule> rules = {
            {L"对称",
             [&](const std::wstring &args, img_fun_type &proc,
                 std::wstring &payload) {
                 int axis = 1;
                 int order = 0;
                 std::wistringstream iss(args);
                 std::wstring token;
                 while (iss >> token) {
                     if (parse_axis_or_order(token, L"axis=", axis) ||
                         parse_axis_or_order(token, L"order=", order)) {
                         continue;
                     }
                     if (!payload.empty()) {
                         payload += L" ";
                     }
                     payload += token;
                 }
                 proc = (img_fun_type){img_fun_type::MIRROR, axis, order};
                 return true;
             }},
            {L"旋转",
             [&](const std::wstring &args, img_fun_type &proc,
                 std::wstring &payload) {
                 int fps = 24;
                 int order = 0;
                 std::wistringstream iss(args);
                 std::wstring token;
                 while (iss >> token) {
                     std::wstring body;
                     if (cmd_strip_prefix(token, L"fps=", body) &&
                         !body.empty()) {
                         fps = std::max(
                             0, std::min(50, (int)my_string2int64(body)));
                         continue;
                     }
                     if (parse_axis_or_order(token, L"order=", order)) {
                         continue;
                     }
                     if (!payload.empty()) {
                         payload += L" ";
                     }
                     payload += token;
                 }
                 proc = (img_fun_type){img_fun_type::ROTATE, fps, order};
                 return true;
             }},
            {L"万花筒",
             [&](const std::wstring &args, img_fun_type &proc,
                 std::wstring &payload) {
                 int layers = 3;
                 int nums = 8;
                 std::wistringstream iss(args);
                 std::wstring token;
                 while (iss >> token) {
                     std::wstring body;
                     if (cmd_strip_prefix(token, L"num=", body) &&
                         !body.empty()) {
                         nums = std::max(
                             3, std::min(12, (int)my_string2int64(body)));
                         continue;
                     }
                     if (!payload.empty()) {
                         payload += L" ";
                     }
                     payload += token;
                 }
                 proc = (img_fun_type){img_fun_type::KALEIDO, layers, nums};
                 return true;
             }},
        };

        for (const auto &r : rules) {
            std::wstring body;
            if (cmd_strip_prefix(input, r.cmd, body)) {
                if (!r.handler) {
                    return false;
                }
                return r.handler(body, out, payload_out);
            }
        }
        return false;
    };

    std::wstring payload = wmessage;
    if (parse_command(wmessage, proc_type, payload)) {
        wmessage = trim(payload);
    }
    else if ((it = is_input.find(conf.user_id)) != is_input.end()) {
        proc_type = is_input[conf.user_id];
    }
    else {
        // ?
        return;
    }

    p.setBar(0.1, "图片处理下载");
    if (wmessage.find(L"[CQ:at") != wmessage.npos) {
        userid_t userid = my_string2uint64(wmessage);
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
        if (it != is_input.end()) {
            is_input.erase(it);
        }
        else {
            conf.p->cq_send("图来", conf);
            is_input[conf.user_id] = proc_type;
        }
        return;
    }

    conf.p->setlog(
        LOG::INFO,
        fmt::format("img_fun at u{} g{}：type={}, para1={}, para2={}",
                    conf.user_id, conf.group_id, (int)proc_type.type,
                    (int)proc_type.para1, (int)proc_type.para2));
    is_input.erase(conf.user_id);
    const std::string download_dir = bot_resource_path(nullptr, "download");
    std::string filepath = download_dir + "/" + filename;
    download(cq_decode(fileurl), download_dir + "/", filename);
    p.setBar(0.2, "图片处理中");
    Magick::Image img;
    bool mgif = false;
    try {
        img.read(filepath);
    }
    catch (...) {
        mgif = true;
    }
    if (img.animationDelay() || proc_type.type == img_fun_type::ROTATE ||
        mgif) {
        std::vector<Magick::Image> img_list;
        Magick::readImages(&img_list, filepath);
        if (proc_type.type == img_fun_type::MIRROR) {
            filename += "_mir.gif";
            float prog = 0.2;
            mirrorImage(
                img_list, proc_type.para1, proc_type.para2,
                [&](float delta_p) { p.setProgress(prog += delta_p * 0.7); });
        }
        else if (proc_type.type == img_fun_type::ROTATE) {
            filename += "_rot.gif";
            float prog = 0.2;
            img_list = rotateImage(
                img, proc_type.para1, proc_type.para2,
                [&](float delta_p) { p.setProgress(prog += delta_p * 0.7); });
        }
        else if (proc_type.type == img_fun_type::KALEIDO) {
            filename += "_kal.gif";
            float prog = 0.2;
            kaleido(
                img_list, proc_type.para1, proc_type.para2,
                [&](float delta_p) { p.setProgress(prog += delta_p * 0.7); });
        }
        p.setBar(0.9, "图片处理完成，保存中");
        Magick::writeImages(img_list.begin(), img_list.end(),
                            download_dir + "/" + filename);
        p.setBar(0.9, "图片处理完成，发送中");
    }
    else {
        if (proc_type.type == img_fun_type::MIRROR) {
            filename += "_mir.png";
            mirrorImage(img, proc_type.para1, proc_type.para2);
        }
        else if (proc_type.type == img_fun_type::KALEIDO) {
            filename += "_kal.png";
            float prog = 0.2;
            kaleido(img, proc_type.para1, proc_type.para2, [&](float delta_p) {
                p.setProgress(prog += delta_p * 0.7);
            });
        }
        img.write(download_dir + "/" + filename);
        p.setBar(0.9, "图片处理完成，发送中");
    }
    conf.p->cq_send(
        "[CQ:image,file=file://" +
            fs::absolute(fs::path(download_dir + "/" + filename)).string() +
            ",id=40000]",
        conf);
    fs::remove(download_dir + "/" + filename);
    fs::remove(filepath);
    conf.p->setlog(LOG::INFO, fmt::format("img_fun done at u{} g{}",
                                          conf.user_id, conf.group_id));
    return;
}
bool img_fun::check(std::string message, const msg_meta &conf)
{
    std::wstring wmes = string_to_wstring(message);
    return message == "img_fun.help" || starts_with(wmes, L"对称 ") ||
           wmes == L"对称" || starts_with(wmes, L"旋转 ") || wmes == L"旋转" ||
           starts_with(wmes, L"万花筒 ") || wmes == L"万花筒" ||
           is_input.find(conf.user_id) != is_input.end();
}
std::string img_fun::help() { return "图片处理。 img_fun.help 获得更多帮助。"; }

DECLARE_FACTORY_FUNCTIONS(img_fun)