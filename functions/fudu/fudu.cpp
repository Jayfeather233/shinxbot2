#include "fudu.h"
#include "utils.h"

#include <map>

void fudu::process(shinx_message msg){
    auto it = gmsg.find(msg.group_id);
    if(it != gmsg.end()){
        if(msg.message == it->second){
            times[msg.group_id] ++;
            if(times[msg.group_id]==5){
                times[msg.group_id] = 0;
                cq_send(msg);
            }
        } else {
            gmsg[msg.group_id] = msg.message;
            times[msg.group_id] = 1;
        }
    } else {
        gmsg[msg.group_id] = msg.message;
        times[msg.group_id] = 1;
    }
}
bool fudu::check(shinx_message msg){
    return msg.message_type == "group" && msg.user_id != get_botqq();
}
std::string fudu::help(){
    return "";
}