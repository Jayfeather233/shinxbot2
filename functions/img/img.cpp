#include "img.h"

#include "utils.h"

img::img(){
    Json::Value J = string_to_json(readfile("./config/img.json", "{}"));
    for(std::string u : J.getMemberNames()){
        images[u] = J[u].asInt64();
    }
}

void img::save(){
    Json::Value J;
    for (auto it : images){
        J[it.first] = it.second;
    }
    writefile("./config/img.json", J.toStyledString());
}

void img::add_image(std::string name, std::string image){
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

void img::commands(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(is_adding[user_id] == true && message.find("[CQ:image,")!=message.npos){
        add_image(add_name[user_id], trim(message));
        is_adding[user_id] = false;
    } else {
        std::wstring wmessage = trim(string_to_wstring(message).substr(3));
        if(wmessage.length() <= 0){
            cq_send("命令错误，使用 \"美图 帮助\" 获取帮助", message_type, user_id, group_id);
        } else if(wmessage.find(L"帮助")==0) {
            cq_send("美图 帮助 - 列出所有美图命令\n"
                    "美图 列表 - 列出所有美图\n"
                    "美图 加入 xxx - 加入一张图片至xxx类\n"
                    "xxx - 发送美图（随机或指定一个数字）", message_type, user_id, group_id);
        } else if(wmessage.find(L"列表")==0) {
            std::ostringstream oss;
            for(auto it : images){
                oss << it.first << '(' << it.second << ")\n";
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
        }
    }
}

void img::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    if(message.find("美图 ") == 0 || (is_adding[user_id] == true && message.find("[CQ:image,")!=message.npos)){
        commands(message, message_type, user_id, group_id);
        return;
    }
    is_adding[user_id] = false;
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