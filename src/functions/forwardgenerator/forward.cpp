#include "forward.h"
#include "utils.h"
#include <iostream>
#include <jsoncpp/json/json.h>

Json::Value forward_msg_gen::get_data(bot *p, std::wstring s1,
                                      std::wistringstream &wiss,
                                      uint64_t group_id)
{
    Json::Value res;
    std::wstring s2;
    std::wstring u;
    wiss >> u;
    getline(wiss, s2);
    s2 = trim(u + s2);
    if (s2 == L"合并行") {
        s2.clear();
        bool flg = false;
        while (!wiss.eof()) {
            std::wstring nxt;
            getline(wiss, nxt);
            nxt = trim(nxt);
            if (nxt == L"结束合并") {
                break;
            }
            if (flg)
                s2 += L'\n';
            s2 += nxt;
            flg = true;
        }
    }
    uint64_t uin = my_string2uint64(s1);
    res["name"] = get_username(p, uin, group_id);
    res["uin"] = std::to_string(uin); //TODO: maybe changed. But now its string
    if (s2 == L"转发") {
        res["content"] = get_content(p, wiss, group_id);
    }
    else {
        size_t pos = 0;
        for (;;) {
            pos = s2.find(L"[CQ:at,qq=", pos);
            if (pos == std::wstring::npos) {
                break;
            }
            size_t po1 = pos + 10;
            while (s2[pos] != L']')
                pos++;
            s2.insert(pos,
                      L",name=" + string_to_wstring(get_username(
                                      p, my_string2uint64(s2.substr(po1, pos - po1)),
                                      group_id)));
        }
        res["content"] = wstring_to_string(s2);
    }
    return res;
}

Json::Value forward_msg_gen::get_content(bot *p, std::wistringstream &wiss,
                                         uint64_t group_id)
{
    std::wstring s1;
    Json::Value Ja;
    while (wiss >> s1) {
        s1 = trim(s1);
        if (s1 == L"结束转发") {
            break;
        }
        Json::Value J;
        J["type"] = "node";
        J["data"] = get_data(p, s1, wiss, group_id);
        Ja.append(J);
    }
    return Ja;
}

void forward_msg_gen::process(std::string message, const msg_meta &conf)
{
    Json::Value J;
    J["message_id"] = conf.message_id;
    conf.p->cq_send("mark_msg_as_read", J);
    std::wstring w_mess = trim(string_to_wstring(message).substr(2));
    if (w_mess == L"帮助") {
        conf.p->cq_send("格式为：\n"
                        "转发\n"
                        "@某人 或 一个qq号 消息（一整行）\n"
                        "@某人 或 一个qq号 合并行\n"
                        "（多行消息）\n"
                        "结束合并\n"
                        "...\n"
                        "@某人 或 一个qq号 转发\n"
                        "（此处为转发内套转发）\n"
                        "结束转发\n"
                        "...\n"
                        "结束转发 ",
                        conf);
        return;
    }
    std::wistringstream wiss(w_mess);
    J.clear();
    J["messages"] = get_content(conf.p, wiss, conf.group_id);

    if (conf.message_type == "group") {
        J["group_id"] = conf.group_id;
        conf.p->cq_send("send_group_forward_msg", J);
        conf.p->setlog(LOG::INFO, "forward_msg_gen at group " +
                                      std::to_string(conf.group_id) + " by " +
                                      std::to_string(conf.user_id));
    }
    else {
        J["user_id"] = conf.user_id;
        conf.p->cq_send("send_private_forward_msg", J);
        conf.p->setlog(LOG::INFO,
                       "forward_msg_gen by " + std::to_string(conf.user_id));
    }
}
bool forward_msg_gen::check(std::string message, const msg_meta &conf)
{
    return string_to_wstring(message).find(L"转发 ") == 0;
}
std::string forward_msg_gen::help()
{
    return "自动生成转发信息： 详细资料请输入 转发帮助";
}
