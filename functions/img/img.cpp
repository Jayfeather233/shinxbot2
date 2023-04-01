#include "img.h"

#include "utils.h"
#include <filesystem>

img::img(){
    Json::Value J = string_to_json(readfile("./config/img.json", "{}"));
    for(std::string u : J.getMemberNames()){
        images[u] = J[u].asInt64();
    }
    parse_json_to_map(J["op_list"], op_list, true);
}

void img::save(){
    Json::Value J;
    for (auto it : images){
        if(it.second != 0) J[it.first] = it.second;
    }
    writefile("./config/img.json", J.toStyledString());
}

void img::add_image(std::string name, std::string image){
    if(name == "op_list") return;
    int index = image.find(",url=");
    index += 5;
    int index2 = index;
    while(image[index2]!=']'){
        ++index2;
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

void img::commands(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(is_deling[user_id]){
        if(message == "y" || message == "Y"){
            del_all(del_name[user_id]);
            cq_send("删除 *所有* "+del_name[user_id], message_type, user_id, group_id);
        } else {
            cq_send("取消删除", message_type, user_id, group_id);
            is_deling[user_id] = false;
        }
    } else if(is_adding[user_id] == true && message.find("[CQ:image,")!=message.npos){
        add_image(add_name[user_id], trim(message));
        is_adding[user_id] = false;
        cq_send("已加入" + add_name[user_id], message_type, user_id, group_id);
    } else {
        std::wstring wmessage = trim(string_to_wstring(message).substr(3));
        if(wmessage.length() <= 1){
            cq_send("命令错误，使用 \"美图 帮助\" 获取帮助", message_type, user_id, group_id);
        } else if(wmessage.find(L"帮助")==0) {
            cq_send("美图 帮助 - 列出所有美图命令\n"
                    "美图 列表 - 列出所有美图\n"
                    "美图 加入 xxx - 加入一张图片至xxx类\n"
                    "xxx - 发送美图（随机或指定一个数字）", message_type, user_id, group_id);
        } else if(wmessage.find(L"列表")==0) {
            std::ostringstream oss;
            for(auto it : images){
                if(it.second != 0) oss << it.first << '(' << it.second << ")\n";
            }
            cq_send(oss.str(), message_type, user_id, group_id);
        } else if(wmessage.find(L"加入")==0){
            std::wstring name = L"";
            int i;
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
                is_adding[user_id] = true;
                add_name[user_id] = wstring_to_string(name);
                cq_send("图来！", message_type, user_id, group_id);
                return;
            } else {
                is_adding[user_id] = false;
                add_image(wstring_to_string(name), wstring_to_string(wmessage));
                cq_send("已加入" + wstring_to_string(name), message_type, user_id, group_id);
            }
        } else if(wmessage.find(L"删除")==0) {
            auto it = op_list.find(user_id);
            if(it == op_list.end() || it->second == false){
                cq_send("Not on op_list.", message_type, user_id, group_id);
            } else {
                std::string name, indexs;
                std::istringstream iss(wstring_to_string(wmessage.substr(2)));
                iss >> name >> indexs;
                if(iss.eof()){
                    cq_send("即将删除 *所有* " + name + "图片，请确认[N/y]", message_type, user_id, group_id);
                    is_deling[user_id] = true;
                    del_name[user_id] = name;
                } else {
                    del_single(name, get_userid(indexs));
                    cq_send("已删除", message_type, user_id, group_id);
                }
            }
        } else {
            cq_send("美图 帮助 - 列出所有美图命令\n"
                    "美图 列表 - 列出所有美图\n"
                    "美图 加入 xxx - 加入一张图片至xxx类\n"
                    "xxx - 发送美图（随机或指定一个数字）", message_type, user_id, group_id);
        }
    }
}

void img::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(message.find("美图 ") == 0 || (is_adding[user_id] == true && message.find("[CQ:image,")!=message.npos) || is_deling[user_id] == true){
        commands(message, message_type, user_id, group_id);
        is_adding[user_id] = false;
        is_deling[user_id] = false;
        return;
    }
    is_adding[user_id] = false;
    is_deling[user_id] = false;
    std::istringstream iss (message);
    std::string name, indexs;
    iss>>name>>indexs;
    auto it = images.find(name);
    if(it == images.end()) return;
    int index;
    if(iss.eof()){
        index = get_random(it->second);
    } else {
        index = get_userid(indexs);
    }
    if(index < 0 || index >= it->second){
        cq_send("索引过大！(0~" + std::to_string(it->second-1) + ")", message_type, user_id, group_id);
        return;
    }
    cq_send("[CQ:image,file=file://" + get_local_path() + "/resource/mt/"+name+"/"+std::to_string(index)+",id=40000]", message_type, user_id, group_id);
}
bool img::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return true;
}
std::string img::help(){
    return "美图： 美图 帮助 - 列出所有美图命令";
}