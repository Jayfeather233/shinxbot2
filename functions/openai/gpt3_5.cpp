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

const int MAX_REPLY = 1000;

gpt3_5::gpt3_5(){
    if(!std::filesystem::exists("./config/openai.json")){
        std::cout<<"Please config your openai key in openai.json (and restart)"<<std::endl;
        std::ofstream of("./config/openai.json", std::ios::out);
        of << 
            "{"
            "\"key\": \"\","
            "\"mode\": [\"default\"],"
            "\"default\": [{\"role\": \"system\", \"content\": \"You are a helpful assistant.\"}],"
            "\"black_list\": [\"股票\"]"
            "}";
        of.close();
    } else {
        std::ifstream afile("./config/openai.json", std::ios::in);
        std::string ans, line;
        while (!afile.eof())
        {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();

        Json::Value res = string_to_json(ans);

        key = res["key"].asString();

        Json::ArrayIndex sz = res["mode"].size();
        for(Json::ArrayIndex i = 0; i < sz; i ++){
            std::string tmp = res["mode"][i].asString();
            modes.insert(tmp);
            mode_prompt[tmp] = res[tmp];
            if(i == 0){
                default_prompt = res[tmp];
            }
        }
        sz = res["op"].size();
        for(Json::ArrayIndex i = 0; i < sz; i ++){
            op_list.insert(res["op"][i].asInt64());
        }
        sz = res["black_list"].size();
        for(Json::ArrayIndex i = 0; i < sz; i ++){
            black_list.insert(res["black_list"][i].asString());
        }
    }
    is_lock = false;
    is_open = true;
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
        auto it = pre_default.find(id);
        if(it!=pre_default.end()){
            history[id] = mode_prompt[it->second];
        } else {
            history[id] = default_prompt;
        }
        cq_send("reset done.", message_type, user_id, group_id);
        return;
    }
    if(message.find(".change")==0){
        if(op_list.find(user_id) != op_list.end()){
            message = trim(message.substr(7));
            auto it = modes.find(message);
            if(it == modes.end()){
                std::string res = "avaliable mdoes:";
                for(std::string u : modes){
                    res += " " + u;
                }
                cq_send(res, message_type, user_id, group_id);
            } else {
                history[id] = mode_prompt[*it];
                pre_default[id] = *it;
                cq_send("change done.", message_type, user_id, group_id);
            }
        } else {
                cq_send("Not on op list.", message_type, user_id, group_id);
        }
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
    if(is_lock){
        cq_send("请等待上次输入的回复。", message_type, user_id, group_id);
        return;
    }
    if(!is_open){
        cq_send("已关闭。", message_type, user_id, group_id);
        return;
    }
    if(history.find(id) == history.end()){
        history[id] = default_prompt;
    }
    std::lock_guard<std::mutex> lock(gptlock);
    is_lock = true;
    Json::Value J, ign;
    J["role"] = "user";
    J["content"] = message;
    history[id].append(J);
    J = history[id];
    while(getlength(J)>4000 - MAX_REPLY){
        J.removeIndex(1, &ign);
    }

    history[id] = J;
    J = Json::Value();
    J["model"] = "gpt-3.5-turbo";
    J["messages"] = history[id];
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
}
bool gpt3_5::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find(".ai") == 0;
}
std::string gpt3_5::help(){
    return "Openai gpt3.5: start with ai.";
}
