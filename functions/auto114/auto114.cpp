#include "auto114.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

auto114::auto114(){
    len = 0;
    
    std::ifstream afile;
    afile.open("./config/homodata.txt", std::ios::in);

    if(afile.is_open()){
        while (!afile.eof()) {
            afile >> ai[len].num;
            afile >> ai[len].ans;
            if(ai[len].ans.size()>0) len++;
        }
        afile.close();
    } else {
        std::cerr<<"Missing file: homodata.txt"<<std::endl;
    }
}

std::string __1 = "(11-4-5+1-4)*";

int auto114::find_min(int64_t input){
    for(int i=0;i<len;i++){
        if(ai[i].num <= input){
            return i;
        }
    }
    return -1;
}

std::string auto114::getans(int64_t input){
    if(input < 0){
        return __1 + "(" + getans(-input) + ")";
    }
    int pos = find_min(input);
    if(pos == -1){
        return "Unexpected error";
    }
    if(input == ai[pos].num){
        return ai[pos].ans;
    } else {
        if(input/ai[pos].num == 1){
            return ai[pos].ans + "+" + getans(input%ai[pos].num);
        } else {
            return "(" + getans(input/ai[pos].num) + ")*(" + ai[pos].ans + ")+" + getans(input%ai[pos].num);
        }
    }
}

void auto114::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    std::istringstream iss(message.substr(4));
    int64_t input;
    iss >> input;
    
    setlog(LOG::INFO, "auto114 at group " + std::to_string(group_id) + " by user " + std::to_string(user_id));
    if(input == 114514){
        cq_send("这么臭的数字有必要论证吗（恼）", message_type, user_id, group_id);
    } else {
        cq_send(std::to_string(input) + "=" + getans(input), message_type, user_id, group_id);
    }
}
bool auto114::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message.find("homo")==0;
}
std::string auto114::help(){
    return "恶臭数字论证器： homo+数字";
}