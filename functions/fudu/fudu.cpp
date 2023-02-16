#include "fudu.h"
#include "utils.h"

#include <map>

std::map<long, std::string> msg;
std::map<long, int> times;

void fudu::process(std::string message, std::string message_type, long user_id, long group_id){
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
bool fudu::check(std::string message, std::string message_type, long user_id, long group_id){
    return message_type == "group" && user_id != get_botqq();
}
std::string fudu::help(){
    return "";
}