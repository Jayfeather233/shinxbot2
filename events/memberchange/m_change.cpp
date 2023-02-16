#include "m_change.h"
#include "utils.h"

#include <iomanip>

void m_change::process(Json::Value J){
    std::string name = get_username(J["user_id"].asInt64(), J["group_id"].asInt64());
    std::ostringstream oss;
    oss << name << " (***" << std::setw(3) << std::setfill('0') << J["user_id"].asInt64()%1000 << ")";
    name = oss.str();

    if(J["notice_type"].asString() == "group_decrease"){
        if(J.isMember("operator_id") && J["operator_id"].asInt64() == J["user_id"].asInt64()){
            cq_send(name + "离开了我们……", "group", 0, J["group_id"].asInt64());
        } else {
            cq_send(name + "被" + get_username(J["operator_id"].asInt64(), J["group_id"].asInt64()) + "送走了", "group", 0, J["group_id"].asInt64());
        }
        setlog(LOG::INFO, "member decrease in group " + std::to_string(J["group_id"].asInt64()) + " by "
            + std::to_string(J["user_id"].asInt64()) + " op " + std::to_string(J["operator_id"].asInt64()));
    } else if(J["notice_type"].asString() == "group_increase"){
        cq_send((std::string)"欢迎" + name + "的加入", "group", 0, J["group_id"].asInt64());
        setlog(LOG::INFO, "member increase in group " + std::to_string(J["group_id"].asInt64()) + " by "
            + std::to_string(J["user_id"].asInt64()));
    }
}
bool m_change::check(Json::Value J){
    if(J["post_type"] != "notice") return false;
    return J["notice_type"].asString() == "group_decrease" || J["notice_type"].asString() == "group_increase";
}