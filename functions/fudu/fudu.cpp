#include "fudu.h"
#include "utils.h"

#include <map>

std::map<int64_t, std::string> msg;
std::map<int64_t, int> times;

void fudu::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    auto it = msg.find(group_id);
    if(it != msg.end()){
        if(message == it->second){
            times[group_id] ++;
            if(times[group_id]==5){
                times[group_id] = 0;
                cq_send(message,message_type,user_id,group_id);
            }
        } else {
            msg[group_id] = message;
            times[group_id] = 1;
        }
    } else {
        msg[group_id] = message;
        times[group_id] = 1;
    }
}
bool fudu::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return message_type == "group" && user_id != get_botqq();
}
std::string fudu::help(){
    return "";
}