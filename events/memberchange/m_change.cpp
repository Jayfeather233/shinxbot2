#include "m_change.h"
#include "utils.h"

#include <iomanip>

void m_change::process(Json::Value J){
    std::string name = get_username(J["user_id"].asInt64(), J["group_id"].asInt64());
    std::ostringstream oss;
    oss << name << " (***" << std::setw(3) << std::setfill('0') << J["user_id"].asInt64()%1000 << ")";
    std::string name1 = oss.str();
    oss.clear();
    oss << name << " (" << J["user_id"].asInt64() << ")";
    std::string name2 = oss.str();

    if(J["notice_type"].asString() == "group_decrease"){
        if(J.isMember("operator_id") && J["operator_id"].asInt64() == J["user_id"].asInt64()){
            cq_send((shinx_message){name1 + " 退群啦", "group", 0, J["group_id"].asInt64(), 0});
        } else {
            cq_send((shinx_message){name2 + " 被 " + get_username(J["operator_id"].asInt64(), J["group_id"].asInt64()) + " 送飞机票啦",
                    "group", 0, J["group_id"].asInt64(), 0});
        }

        setlog(LOG::INFO, "member decrease in group " + std::to_string(J["group_id"].asInt64()) + " by "
            + std::to_string(J["user_id"].asInt64()) + " op " + std::to_string(J["operator_id"].asInt64()));
    } else if(J["notice_type"].asString() == "group_increase"){
        cq_send((shinx_message){(std::string)"欢迎" + name1 + "的加入",
                "group", 0, J["group_id"].asInt64(), 0});
        
        setlog(LOG::INFO, "member increase in group " + std::to_string(J["group_id"].asInt64()) + " by "
            + std::to_string(J["user_id"].asInt64()));
    }
}
bool m_change::check(Json::Value J){
    if(J["post_type"] != "notice") return false;
    return J["notice_type"].asString() == "group_decrease" || J["notice_type"].asString() == "group_increase";
}