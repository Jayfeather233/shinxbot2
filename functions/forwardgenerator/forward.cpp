#include "forward.h"
#include "utils.h"

#include <iostream>

#include <jsoncpp/json/json.h>

Json::Value forward::get_data(std::wstring s1, std::wistringstream &wiss, long group_id){
    Json::Value res;
    std::wstring s2;
    std::wstring u;
    wiss >> u;
    getline(wiss, s2);
    s2 = trim(u+s2);
    if(s2 == L"合并行"){
        s2.clear();
        bool flg = false;
        while(!wiss.eof()){
            std::wstring nxt;
            getline(wiss, nxt);
            nxt = trim(nxt);
            if(nxt == L"结束合并"){
                break;
            }
            if(flg) s2 += L'\n';
            s2 += nxt;
            flg = true;
        }
    }
    long uin = get_userid(s1);
    res["name"] = get_username(uin, group_id);
    res["uin"] = uin;
    if(s2 == L"转发") {
        res["content"] = get_content(wiss, group_id);
    } else {
        size_t pos = 0;
        for(;;){
            pos = s2.find(L"[CQ:at,qq=", pos);
            if (pos == std::wstring::npos){
                break;
            }
            int po1 = pos + 10;
            while(s2[pos] != L']') pos++;
            s2.insert(pos, L",name=" + string_to_wstring(get_username(get_userid(s2.substr(po1, pos-po1)),group_id)));
        }
        res["content"] = wstring_to_string(s2);
    }
    return res;
}

Json::Value forward::get_content(std::wistringstream &wiss, long group_id){
    std::wstring s1;
    Json::Value Ja;
    while(wiss >> s1){
        s1 = trim(s1);
        if(s1 == L"结束转发"){
            break;
        }
        Json::Value J;
        J["type"] = "node";
        J["data"] = get_data(s1, wiss, group_id);
        Ja.append(J);
    }
    return Ja;
}

void forward::process(std::string message, std::string message_type, long user_id, long group_id){
    std::wstring w_mess = string_to_wstring(message).substr(2);
    if(w_mess == L"帮助"){
        cq_send("格式为：\n转发\n[@某人或qq号] 消息（一整行）\n[@某人或qq号] 合并行\n（多行消息）\n结束合并\n...\n[@某人或qq号] 转发\n（此处为转发内套转发）\n结束转发\n...\n结束转发 ",
            message_type,user_id,group_id);
        return;
    }
    std::wistringstream wiss(w_mess);
    Json::Value J;
    J["messages"] = get_content(wiss, group_id);

    if(message_type == "group"){
        J["group_id"] = group_id;
        cq_send("send_group_forward_msg", J);
    } else {
        J["user_id"] = group_id;
        cq_send("send_private_forward_msg", J);
    }
}
bool forward::check(std::string message, std::string message_type, long user_id, long group_id){
    return string_to_wstring(message).find(L"转发") == 0;
}
std::string forward::help(){
    return "自动生成转发信息： 详细资料请输入 转发帮助";
}