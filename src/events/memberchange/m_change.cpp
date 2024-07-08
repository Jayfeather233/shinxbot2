#include "m_change.h"
#include "utils.h"

#include <iomanip>

void m_change::process(bot *p, Json::Value J)
{
    std::string name =
        get_username(p, J["user_id"].asUInt64(), J["group_id"].asUInt64());
    std::ostringstream oss;
    oss << name << " (***" << std::setw(3) << std::setfill('0')
        << J["user_id"].asUInt64() % 1000 << ")";
    std::string name1 = oss.str();
    oss = std::ostringstream();
    oss << name << " (" << J["user_id"].asUInt64() << ")";
    std::string name2 = oss.str();

    if (J["notice_type"].asString() == "group_decrease") {
        // if (J.isMember("operator_id") &&
        //     J["operator_id"].asInt64() == J["user_id"].asInt64()) {
        //     ;
        // }
        // else if (J.isMember("operator_id")) {
        //     cq_send(name2 + " 被 " +
        //                 get_username(J["operator_id"].asInt64(),
        //                              J["group_id"].asInt64()) +
        //                 " 送飞机票啦",
        //             (msg_meta){"group", 0, J["group_id"].asInt64(), 0});
        // }
        // Just ignore this ...

        p->setlog(LOG::INFO, "member decrease in group " +
                              std::to_string(J["group_id"].asUInt64()) + " by " +
                              std::to_string(J["user_id"].asUInt64()) + " op " +
                              std::to_string(J["operator_id"].asUInt64()));
    }
    else if (J["notice_type"].asString() == "group_increase") {
        p->cq_send((std::string) "欢迎" + name1 + "的加入",
                (msg_meta){"group", 0, J["group_id"].asUInt64(), 0});

        p->setlog(LOG::INFO, "member increase in group " +
                              std::to_string(J["group_id"].asUInt64()) + " by " +
                              std::to_string(J["user_id"].asUInt64()));
    }
}
bool m_change::check(bot *p, Json::Value J)
{
    if (J["post_type"] != "notice")
        return false;
    return J["notice_type"].asString() == "group_decrease" ||
           J["notice_type"].asString() == "group_increase";
}

extern "C" eventprocess* create() {
    return new m_change();
}