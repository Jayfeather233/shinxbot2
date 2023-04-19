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
int RED_LINE = 1000;
const int MAX_KEYS = 255;

std::mutex gptlock[MAX_KEYS];

gpt3_5::gpt3_5(){
    if(!std::filesystem::exists("./config/openai.json")){
        std::cout<<"Please config your openai key in openai.json (and restart) OR see openai_example.json"<<std::endl;
        std::ofstream of("./config/openai.json", std::ios::out);
        of << 
            "{"
            "\"keys\": [\"\"],"
            "\"mode\": [\"default\"],"
            "\"default\": [{\"role\": \"system\", \"content\": \"You are a helpful assistant.\"}],"
            "\"black_list\": [\"股票\"],"
            "\"MAX_TOKEN\": 4000,"
            "\"MAX_REPLY\": 700,"
            "\"RED_LINE\": 1000,"
            "}";
        of.close();
    } else {
        std::string ans = readfile("./config/openai.json");

        Json::Value res = string_to_json(ans);

        Json::ArrayIndex sz = res["keys"].size();
        for(Json::ArrayIndex i = 0; i < sz; ++i){
            key.push_back(res["keys"][i].asString());
            is_lock.push_back(false);
        }

        sz = res["mode"].size();
        for(Json::ArrayIndex i = 0; i < sz; i ++){
            std::string tmp = res["mode"][i].asString();
            modes.push_back(tmp);
            mode_prompt[tmp] = res[tmp];
            if(i == 0){
                default_prompt = tmp;
            }
        }

        parse_json_to_set(res["black_list"], black_list);

        MAX_TOKEN = res["MAX_TOKEN"].asInt();
        MAX_REPLY = res["MAX_REPLY"].asInt();
        RED_LINE = res["RED_LINE"].asInt();
    }
    is_open = true;
    key_cycle = 0;

    if(std::filesystem::exists("./config/gpt3_5")){
        std::filesystem::path gpt_files = "./config/gpt3_5";
        std::filesystem::directory_iterator di(gpt_files);
        for(auto &entry : di){
            if(entry.is_regular_file()){
                std::istringstream iss(entry.path().filename().string());
                int64_t id;
                iss>>id;
                Json::Value J = string_to_json(readfile(entry.path()));
                history[id] = J["history"];
                pre_default[id] = J["pre_prompt"].asString();
            }
        }
    }
}

void gpt3_5::save_file(){
    Json::Value J;
    for(std::string u : key)
        J["keys"].append(u);
    for(std::string u : modes)
        J["mode"].append(u);
    J["black_list"] = parse_set_to_json(black_list);
    J["MAX_TOKEN"] = MAX_TOKEN;
    J["MAX_REPLY"] = MAX_REPLY;
    J["RED_LINE"] = RED_LINE;
    for(std::string u : modes){
        J[u] = mode_prompt[u];
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

size_t gpt3_5::get_avaliable_key(){
    size_t u;
    for(int i = 0; i < key.size(); i++){
        if(!is_lock[u = (key_cycle + i)%key.size()]){
            key_cycle = u;
            return u;
        }
    }
    return key_cycle;
}

void gpt3_5::save_history(int64_t id){
    Json::Value J;
    J["pre_prompt"] = pre_default[id];
    J["history"] = history[id];
    writefile("./config/gpt3_5/" + std::to_string(id) + ".json", J.toStyledString());
}

void gpt3_5::process(shinx_message msg){
    msg.message = do_black(trim(msg.message.substr(3)));
    if(msg.message.find(".test") == 0){
        cq_send(msg);
        return;
    }
    int64_t id = msg.message_type == "group" ? (msg.group_id<<1) : ((msg.user_id<<1)|1);
    if(msg.message.find(".reset") == 0 || msg.message.find("reset") == 0){
        auto it = history.find(id);
        if(it!=history.end()){
            it->second.clear();
        }
        msg.message = "reset done.";
        cq_send(msg);
        save_history(id);
        return;
    }
    if(msg.message.find(".change")==0){
        if(is_op(msg.user_id) || (id&1)){
            msg.message = trim(msg.message.substr(7));
            bool flg = 0;
            std::string res = "avaliable modes:";
            for(std::string u : modes){
                res += " " + u;
                if(u == msg.message){
                    flg = true;
                    history[id].clear();
                    pre_default[id] = msg.message;
                    msg.message = "change done.";
                    cq_send(msg);
                    break;
                }
            }
            if(!flg){
                msg.message = res;
                cq_send(msg);
            }
        } else {
            msg.message = "Not on op list.";
            cq_send(msg);
        }
        save_history(id);
        return;
    }
    if(msg.message.find(".sw")==0){
        if(is_op(msg.user_id)){
            is_open = !is_open;
            close_message = trim(msg.message.substr(3));
            msg.message = "is_open: " + std::to_string(is_open);
            cq_send(msg);
        } else {
            msg.message = "Not on op list.";
            cq_send(msg);
        }
        return;
    }
    if(msg.message.find(".debug")==0){
        if(is_op(msg.user_id)){
            is_debug = !is_debug;
            msg.message = "is_debug: " + std::to_string(is_debug);
            cq_send(msg);
        } else {
            msg.message = "Not on op list.";
            cq_send(msg);
        }
        return;
    }
    if(msg.message.find(".set") == 0){
        if(is_op(msg.user_id)){
            std::string type;
            int64_t num;
            std::istringstream iss(msg.message.substr(4));
            iss >> type >> num;
            if(type == "reply"){
                MAX_REPLY = num;
                msg.message = "set MAX_REPLY to " + std::to_string(num);
            } else if(type == "token"){
                MAX_TOKEN = num;
                msg.message = "set MAX_TOKEN to " + std::to_string(num);
            } else if(type == "red"){
                RED_LINE = num;
                msg.message = "set RED_LINE to " + std::to_string(num);
            } else {
                msg.message = "Unknown type";
            }
        } else {
            msg.message = "Not on op list.";
        }
        cq_send(msg);
        save_file();
        return;
    }
    size_t keyid = get_avaliable_key();
    if(is_lock[keyid]){
        msg.message = "请等待上次输入的回复。";
        cq_send(msg);
        return;
    }
    if(!is_open){
        msg.message = "已关闭。" + close_message;
        cq_send(msg);
        return;
    }
    if(history.find(id) == history.end()){
        pre_default[id] = default_prompt;
    }
    std::lock_guard<std::mutex> lock(gptlock[keyid]);
    is_lock[keyid] = true;
    Json::Value J, user_input_J, ign;
    user_input_J["role"] = "user";
    user_input_J["content"] = msg.message;
    // J = history[id];
    // while(getlength(J) > MAX_TOKEN - MAX_REPLY){
    //     J.removeIndex(0, &ign);
    //     J.removeIndex(0, &ign);
    // }

    // history[id] = J;
    // J = Json::Value();
    J["model"] = "gpt-3.5-turbo-0301";
    Json::Value K = mode_prompt[pre_default[id]];
    auto it = history.find(id);
    if(it!=history.end()){
        Json::ArrayIndex sz = it->second.size();
        for(Json::ArrayIndex i = 0; i < sz; i++){
            K.append(it->second[i]);
        }
    }
    K.append(user_input_J);
    J["messages"] = K;
    J["temperature"] = 0.7;
    J["max_tokens"] = MAX_REPLY;
    try{
        J = string_to_json(do_post("https://api.openai.com/v1/chat/completions", J, {{"Content-Type","application/json"},{"Authorization", "Bearer " + key[keyid]}}, true));
    } catch (std::string e){
        J.clear();
        J["error"]["message"] = e;
    } catch (...){
        J.clear();
        J["error"]["message"] = "http connection failed.";
    }
    setlog(LOG::INFO, "openai: user " + std::to_string(msg.user_id));
    is_lock[keyid] = false;
    if(J.isMember("error")){
        if(J["error"]["message"].asString().find("This model's maximum context length is") == 0){
            msg.message = "Openai ERROR: history message is too long. Please try again or try .ai.reset";
            cq_send(msg);
            history[id].removeIndex(0, &ign);
            history[id].removeIndex(0, &ign);
            save_history(id);
        } else {
            msg.message = "Openai ERROR: " + J["error"]["message"].asString();
            cq_send(msg);
        }
    } else {
        std::string aimsg = J["choices"][0]["message"]["content"].asString();
        aimsg = do_black(aimsg);

        int tokens = static_cast<int>(J["usage"]["total_tokens"].asInt64());

        if(MAX_TOKEN < tokens){
            history[id].clear();
        } else {
            for(int i = 5;i>=1;i--){
                if(MAX_TOKEN - RED_LINE / i < tokens){
                    for(int j = 0; j < i; j++){
                        history[id].removeIndex(0, &ign);
                        history[id].removeIndex(0, &ign);
                    }
                    break;
                }
            }
        }
        
        std::string usage = "\n" + J["usage"].toStyledString();
        J.clear();
        J["role"] = "assistant";
        J["content"] = aimsg;
        if(is_debug) aimsg += usage;
        msg.message = "[CQ:reply,id=" + std::to_string(msg.message_id) + "] " + aimsg;
        cq_send(msg);
        history[id].append(user_input_J);
        history[id].append(J);
    }
    save_history(id);
}
bool gpt3_5::check(shinx_message msg){
    return msg.message.find(".ai") == 0;
}
std::string gpt3_5::help(){
    return "Openai gpt3.5: start with .ai";
}
