#include "img.h"

#include "utils.h"
#include <filesystem>
#include <iostream>

img::img(){
    Json::Value J = string_to_json(readfile("./config/img.json", "{}"));
    for(std::string u : J.getMemberNames()){
        if(u != "belongs" && u != "default")
            images[u] = J[u].asInt64();
    }
    for(std::string u : J["belongs"].getMemberNames()){
        belongs[get_userid(u)] = J["belongs"][u];
    }
    parse_json_to_set(J["default"], default_img);
}

void img::save(){
    Json::Value J;
    for (auto it : images){
        if(it.second != 0) J[it.first] = it.second;
    }
    for(auto it : belongs){
        J["belongs"][std::to_string(it.first)] = it.second;
    }
    J["default"] = parse_set_to_json(default_img);
    writefile("./config/img.json", J.toStyledString());
}

void img::add_image(std::string name, std::string image, int64_t group_id){
    if(name == "belongs" || name == "default") return;
    int index = image.find(",url=");
    index += 5;
    int index2 = index;
    while(image[index2]!=']'){
        ++index2;
    }
    auto it = images.find(name);
    if(it==images.end()){
        auto it2 = belongs.find(group_id);
        if(it2 == belongs.end()){
            for(std::string u : default_img)
                belongs[group_id].append(u);
        }
        belongs[group_id].append(name);
    }
    download(image.substr(index, index2-index), "./resource/mt/" + name, std::to_string(images[name]));
    images[name] ++;
    save();
}

void img::del_all(std::string name){
    std::filesystem::remove_all("./resource/mt/" + name);
    images[name] = 0;
    save();
}
void img::del_single(std::string name, int index){
    std::string prefix = "./resource/mt/" + name + "/";
    std::filesystem::remove(prefix + std::to_string(index));
    for(int i=index+1;i<images[name];i++){
        std::filesystem::rename(prefix + std::to_string(i),prefix + std::to_string(i-1));
    }
    images[name] --;
    save();
}

std::string img::commands(shinx_message msg){
    // std::cout<<message<<std::endl;
    if(is_deling[msg.user_id]){
        if(msg.message == "y" || msg.message == "Y"){
            del_all(del_name[msg.user_id]);
            cq_send(msg);
            is_deling[msg.user_id] = false;
            return "删除 *所有* "+del_name[msg.user_id];
        } else {
            is_deling[msg.user_id] = false;
            return "取消删除";
        }
    } else if(is_adding[msg.user_id] == true && msg.message.find("[CQ:image,")!=msg.message.npos){
        add_image(add_name[msg.user_id], trim(msg.message), msg.group_id);
        is_adding[msg.user_id] = false;
        return "已加入" + add_name[msg.user_id];
    } else {
        std::wstring wmessage = trim(string_to_wstring(msg.message).substr(3));
        if(wmessage.length() <= 1){
            return "命令错误，使用 \"美图 帮助\" 获取帮助";
        } else if(wmessage.find(L"帮助")==0) {
           return   "美图 帮助 - 列出所有美图命令\n"
                    "美图 列表 - 列出本群美图\n"
                    "美图 加入 xxx - 加入一张图片至xxx类\n"
                    "xxx - 发送美图（随机或指定一个数字）";
        } else if(wmessage.find(L"列表")==0) {
            std::ostringstream oss;
            if(wmessage.find(L"all") != wmessage.npos && is_op(msg.user_id)){
                for(auto it : images){
                    if(it.second != 0) oss << it.first << '(' << it.second << ")\n";
                }
            } else {
                auto it = belongs.find(msg.group_id);
                if(it == belongs.end()){
                    for(auto it2 : default_img){
                        if(images[it2] != 0) oss << it2 << '(' << images[it2] << ")\n";
                    }
                } else {
                    for(auto it2 : belongs[msg.group_id]){
                        if(images[it2.asString()] != 0) oss << it2.asString() << '(' << images[it2.asString()] << ")\n";
                    }
                }
            }
            return oss.str();
        } else if(wmessage.find(L"加入")==0){
            std::wstring name = L"";
            size_t i;
            wmessage = trim(wmessage.substr(2));
            for(i = 0; i < wmessage.length(); i++){
                if(wmessage[i] == L' ' || wmessage[i] == L'['){
                    break;
                }
                name += wmessage[i];
            }
            name = trim(name);
            wmessage = trim(wmessage.substr(i));
            if(wmessage.length() <= 1){
                is_adding[msg.user_id] = true;
                add_name[msg.user_id] = wstring_to_string(name);
                return "图来！";
            } else {
                is_adding[msg.user_id] = false;
                add_image(wstring_to_string(name), wstring_to_string(wmessage), msg.group_id);
                return "已加入" + wstring_to_string(name);
            }
        } else if(wmessage.find(L"删除")==0) {
            if(!is_op(msg.user_id)){
                return "Not on op_list.";
            } else {
                std::string name, indexs;
                std::istringstream iss(wstring_to_string(wmessage.substr(2)));
                iss >> name >> indexs;
                if(indexs.length() <= 0){
                    is_deling[msg.user_id] = true;
                    del_name[msg.user_id] = name;
                    return "即将删除 *所有* " + name + "图片，请确认[N/y]";
                } else {
                    del_single(name, get_userid(indexs)-1);
                    return "已删除";
                }
            }
        } else {
            return  "美图 帮助 - 列出所有美图命令\n"
                    "美图 列表 - 列出本群美图\n"
                    "美图 加入 xxx - 加入一张图片至xxx类\n"
                    "xxx - 发送美图（随机或指定一个数字）";
        }
    }
}

bool is_member(Json::Value J, std::string u){
    for(auto it : J){
        if(u == it.asString()) return true;
    }
    return false;
}

void img::process(shinx_message msg){
    if(msg.message.find("美图 ") == 0 || (is_adding[msg.user_id] == true && msg.message.find("[CQ:image,")!=msg.message.npos) || is_deling[msg.user_id] == true){
        msg.message = commands(msg);
        cq_send(msg);
        return;
    }
    is_adding[msg.user_id] = false;
    is_deling[msg.user_id] = false;
    std::istringstream iss (msg.message);
    std::string name, indexs;
    iss>>name>>indexs;
    std::map<std::string, int64_t>::iterator it2;
    if(msg.message_type == "group"){
        auto it1 = belongs.find(msg.group_id);
        if(it1 == belongs.end()){
            if(default_img.find(name) == default_img.end()){
                it2 = images.end();
            } else {
                it2 = images.find(name);
            }
        } else {
            if(is_member(belongs[msg.group_id], name)){
                it2 = images.find(name);
            } else {
                it2 = images.end();
            }
        }
    } else {
        it2 = images.find(name);
    }
    if(it2 == images.end()) return;
    int index;
    if(indexs.length() < 1){
        index = get_random(it2->second) + 1;
    } else {
        index = get_userid(indexs);
    }
    index --;
    if(index < 0 || index >= it2->second){
        msg.message = "索引越界！(1~" + std::to_string(it2->second) + ")";
        cq_send(msg);
        return;
    }
    msg.message = "[CQ:image,file=file://" + get_local_path() + "/resource/mt/"+name+"/"+std::to_string(index)+",id=40000]";
    setlog(LOG::INFO, "img at group " + std::to_string(msg.group_id));
    cq_send(msg);
}
bool img::check(shinx_message msg){
    return true;
}
std::string img::help(){
    return "美图： 美图 帮助 - 列出所有美图命令";
}