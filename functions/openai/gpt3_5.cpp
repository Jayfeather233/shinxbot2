#include "gpt3_5.h"
#include "utils.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>

/**
 * Overall API intro: https://platform.openai.com/docs/api-reference/chat/create
 * do post at https://api.openai.com/v1/chat/completions
 * model: gpt-3.5-turbo
 * upload:
 * {
 *  "model": "gpt-3.5-turbo",
 *  "messages": [{"role": "user", "content": "Hello!"}]
 * }
 * messages format: https://platform.openai.com/docs/guides/chat/introduction
 * temperature: 0~1
 * max_tokens: default inf(4096)
 * 
 * ATTENTION!!!
 * In general, gpt-3.5-turbo-0301 does not pay strong attention to the system
 * message, and therefore important instructions are often better placed in a user message.
*/

int MAX_TOKEN = 4000;
int MAX_REPLY = 1000;

gpt3_5::gpt3_5(){
    if(!std::filesystem::exists("./config/openai.json")){
        std::cout<<"Please config your openai key in openai.json (and restart) OR see openai_example.json"<<std::endl;
        std::ofstream of("./config/openai.json", std::ios::out);
        of << 
            "{"
            "\"key\": \"\","
            "\"mode\": [\"default\"],"
            "\"default\": [{\"role\": \"system\", \"content\": \"You are a helpful assistant.\"}],"
            "\"black_list\": [\"股票\"],"
            "\"MAX_TOKEN\": 4000,"
            "\"MAX_REPLY\": 700,"
            "}";
        of.close();
    } else {
        std::string ans = readfile("./config/openai.json");

        Json::Value res = string_to_json(ans);

        key = res["key"].asString();

        Json::ArrayIndex sz = res["mode"].size();
        modes = res["mode"];
        for(Json::ArrayIndex i = 0; i < sz; i ++){
            std::string tmp = res["mode"][i].asString();
            mode_prompt[tmp] = res[tmp];
            if(i == 0){
                default_prompt = tmp;
            }
        }

        parse_json_to_set(res["op"], op_list);
        parse_json_to_set(res["black_list"], black_list);

        MAX_TOKEN = res["MAX_TOKEN"].asInt();
        MAX_REPLY = res["MAX_REPLY"].asInt();
    }
    is_lock = false;
    is_open = true;

    if(std::filesystem::exists("./config/gpt3_5")){
        std::filesystem::path gpt_files = "./config/gpt3_5";
        std::filesystem::directory_iterator di(gpt_files);
        for(auto &entry : di){
            if(entry.is_regular_file()){
                std::istringstream iss(entry.path().filename().string());
                int64_t id;
                iss>>id;
                history[id] = string_to_json(readfile(entry.path()));
            }
        }
    }
}

void gpt3_5::save_file(){
    Json::Value J;
    J["key"] = key;
    J["mode"] = modes;
    J["op"] = parse_set_to_json(op_list);
    J["black_list"] = parse_set_to_json(black_list);
    J["MAX_TOKEN"] = MAX_TOKEN;
    J["MAX_REPLY"] = MAX_REPLY;
    Json::ArrayIndex sz = modes.size();
    for(Json::ArrayIndex i = 0; i < sz; i ++){
        J[modes[i].asString()] = mode_prompt[modes[i].asString()];
    }
    writefile("./config/openai.json", J.toStyledString());
}

int64_t getlength(const Json::Value &J){
    int64_t l = 0;
    Json::ArrayIndex sz = J.size();
    for(Json::ArrayIndex i = 0; i < sz; i++){
        l += J[i]["content"].asString().length() / 0.75;
    }
    return l;
}

std::mutex gptlock;

std::string gpt3_5::do_black(std::string msg){
    std::string p1,p2;
    for(std::string u : black_list){
        size_t pos = msg.find(u);
        while(pos != std::string::npos){
            p1 = msg.substr(0,pos);
            p2 = msg.substr(pos + u.length());
            msg = p1 + "__" + p2;
            pos = msg.find(u, pos);
        }
    }
    return msg;
}

void gpt3_5::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    message = trim(message.substr(3));
    message = do_black(message);
    if(message.find(".test") == 0){
        cq_send(message, message_type, user_id, group_id);
        return;
    }
    int64_t id = message_type == "group" ? (group_id<<1) : ((user_id<<1)|1);
    if(message.find(".reset") == 0){
        auto it = history.find(id);
        if(it!=history.end()){
            it->second.clear();
        }
        cq_send("reset done.", message_type, user_id, group_id);
        return;
    }
    if(message.find(".change")==0){
        if(op_list.find(user_id) != op_list.end() || (id&1)){
            message = trim(message.substr(7));
            Json::ArrayIndex sz = modes.size();
            bool flg = 0;
            std::string res = "avaliable modes:";
            for(Json::ArrayIndex i =0; i < sz; i++){
                res += " " + modes[i].asString();
                if(modes[i].asString() == message){
                    flg = true;
                    history[id].clear();
                    pre_default[id] = message;
                    cq_send("change done.", message_type, user_id, group_id);
                    break;
                }
            }
            if(!flg){
                cq_send(res, message_type, user_id, group_id);
            }
        } else {
                cq_send("Not on op list.", message_type, user_id, group_id);
        }
        writefile("./config/gpt3_5/" + std::to_string(id) + ".json", history[id].toStyledString());
        return;
    }
    if(message.find(".sw")==0){
        if(op_list.find(user_id) != op_list.end()){
            is_open = !is_open;
            cq_send("is_open: " + std::to_string(is_open), message_type, user_id, group_id);
        } else {
            cq_send("Not on op list.", message_type, user_id, group_id);
        }
        return;
    }
    if(message.find(".set") == 0){
        if(op_list.find(user_id) != op_list.end()){
            std::string type;
            int64_t num;
            std::istringstream iss(message.substr(4));
            iss >> type >> num;
            if(type == "reply"){
                MAX_REPLY = num;
                cq_send("set MAX_REPLY to " + std::to_string(num), message_type, user_id, group_id);
            } else if(type == "token"){
                MAX_TOKEN = num;
                cq_send("set MAX_TOKEN to " + std::to_string(num), message_type, user_id, group_id);
            } else {
                cq_send("Unknown type", message_type, user_id, group_id);
            }
        } else {
            cq_send("Not on op list.", message_type, user_id, group_id);
        }
        save_file();
        return;
    }
    if(is_lock){
        cq_send("请等待上次输入的回复。", message_type, user_id, group_id);
        return;
    }
    if(!is_open){
        cq_send("已关闭。", message_type, user_id, group_id);
        return;
    }
    if(history.find(id) == history.end()){
        pre_default[id] = default_prompt;
    }
    std::lock_guard<std::mutex> lock(gptlock);
    is_lock = true;
    Json::Value J, ign;
    J["role"] = "user";
    J["content"] = message;
    history[id].append(J);
    J = history[id];
    while(getlength(J) > MAX_TOKEN - MAX_REPLY){
        J.removeIndex(0, &ign);
    }

    history[id] = J;
    J = Json::Value();
    J["model"] = "gpt-3.5-turbo";
    Json::Value K = mode_prompt[pre_default[id]];
    auto it = history.find(id);
    if(it!=history.end()){
        Json::ArrayIndex sz = it->second.size();
        for(Json::ArrayIndex i = 0; i < sz; i++){
            K.append(it->second[i]);
        }
    }
    J["messages"] = K;
    J["temperature"] = 0.7;
    J["max_tokens"] = MAX_REPLY;
    try{
        J = string_to_json(do_post("https://api.openai.com/v1/chat/completions", J, {{"Content-Type","application/json"},{"Authorization", "Bearer " + key}}, true));
    } catch (...){
        J.clear();
        J["error"]["message"] = "http connection failed.";
    }
    setlog(LOG::INFO, "openai: user " + std::to_string(user_id));
    is_lock = false;
    if(J.isMember("error")){
        cq_send("Openai ERROR: " + J["error"]["message"].asString(), message_type, user_id, group_id);
    }else{
        std::string msg = J["choices"][0]["message"]["content"].asString();
        msg = do_black(msg);
        cq_send(msg, message_type, user_id, group_id);
        J.clear();
        J["role"] = "assistant";
        J["content"] = msg;
        history[id].append(J);
    }
    writefile("./config/gpt3_5/" + std::to_string(id) + ".json", history[id].toStyledString());
}
bool gpt3_5::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find(".ai") == 0;
}
std::string gpt3_5::help(){
    return "Openai gpt3.5: start with .ai";
}
