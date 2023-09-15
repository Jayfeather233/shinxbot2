#include "gray_list.h"
#include "utils.h"

gray_list::gray_list(){
    Json::Value J = string_to_json( readfile("./config/g_list.json", "{}") );
    for(std::string group_id_s : J.getMemberNames()){
        int64_t group_id = get_userid(group_id_s);
        g_list[group_id] = J[group_id_s];
    }
}

void gray_list::save(){
    Json::Value J;
    for(auto it : g_list){
        J[std::to_string(it.first)] = it.second;
    }
    writefile("./config/g_list.json", J.toStyledString());
}

void gray_list::process(std::string message, const msg_meta &conf){
    message = trim(message);
    int64_t user_id = get_userid(message);

    std::string user_name = get_username(conf.p, user_id, conf.group_id);
    Json::ArrayIndex ind;
    if((ind = json_array_find(g_list[conf.group_id], user_id))!=-1){
        Json::Value Jdata;
        Jdata["group_id"] = conf.group_id;
        Jdata["user_id"] = user_id;
        Json::Value res = string_to_json(conf.p->cq_send("set_group_kick", Jdata));
        if(res["status"].asString() == "failed"){
            msg_meta s_conf;
            s_conf.group_id = -1;
            s_conf.message_type = "private";
            s_conf.user_id = conf.user_id;
            conf.p->cq_send("无法踢出\nmsg: " + res["wording"].asString(), s_conf);
        } else {
            conf.p->cq_send("两次添加灰名单，踢出 " + user_name, conf);
        }
        Json::Value ign;
        g_list[conf.group_id].removeIndex(ind, &ign);
    } else {
        Json::Value res = string_to_json(conf.p->cq_send("将 " + user_name + " 添加进灰名单", conf));
        if(res["status"].asString() == "failed"){
            msg_meta s_conf;
            s_conf.group_id = -1;
            s_conf.message_type = "private";
            s_conf.user_id = conf.user_id;
            conf.p->cq_send("将 " + user_name + " 添加进灰名单\nmsg: " + res["wording"].asString(), s_conf);
        }
        g_list[conf.group_id].append(user_id);
    }
    save();
}
bool gray_list::check(std::string message, const msg_meta &conf){
    return (message.find("添加灰名单") == 0 || message.find("加入灰名单") == 0) && conf.message_type == "group" && is_group_op(conf.p, conf.group_id, conf.user_id);
}
std::string gray_list::help(){
    return "群灰名单：使用 加入灰名单 @xxx 使用";
}
