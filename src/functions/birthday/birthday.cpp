#include "birthday.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>

static std::string birth_help_msg = "\
date.add MMDD event\n\
date.del event\n\
date.list\
";

birthday::birthday()
{
    Json::Value Ja = string_to_json(readfile("./config/birthday.json", "{}"));
    for (const std::string &uid : Ja.getMemberNames()) {
        groupid_t uuid = std::stoull(uid);
        if (uuid == 0) {
            for (const auto &J : Ja[uid]) {
                inform_interval.insert(J.asInt());
            }
        }
        else {
            for (const auto &J : Ja[uid]) {
                birthdays[uuid].push_back((mmdd){
                    J["who"].asString(), J["mm"].asInt(), J["dd"].asInt()});
            }
        }
    }
}
void birthday::save()
{
    Json::Value Jaa(Json::objectValue);
    for (const auto &it : birthdays) {
        Json::Value Ja(Json::arrayValue);
        for (const auto &it2 : it.second) {
            Json::Value J;
            J["mm"] = it2.mm;
            J["dd"] = it2.dd;
            J["who"] = it2.name;
            Ja.append(J);
        }
        Jaa[std::to_string(it.first)] = Ja;
    }
    Jaa["0"] = parse_set_to_json(inform_interval);
    writefile("./config/birthday.json", Jaa.toStyledString());
}

/*
birthday.add mmdd who
birthday.del who
*/
void birthday::process(std::string message, const msg_meta &conf)
{
    std::istringstream iss(trim(message));
    std::string command;
    iss >> command;

    if (command == "date.add") {
        std::string who, date;
        iss >> date;
        getline(iss, who);
        who = trim(who);
        if (date.size() == 4 && !who.empty()) {
            mmdd u;
            try {
                u = (mmdd){who, std::stoi(date.substr(0, 2)),
                           std::stoi(date.substr(2, 2))};
            }
            catch (...) {
                conf.p->cq_send("日期不是数字", conf);
                return;
            }
            if (check_valid_date(u)) {
                birthdays[conf.group_id].push_back(u);

                std::sort(birthdays[conf.group_id].begin(),
                          birthdays[conf.group_id].end(),
                          [](const mmdd &a, const mmdd &b) {
                              return (a.mm < b.mm) ||
                                     (a.mm == b.mm && a.dd < b.dd);
                          });
                conf.p->cq_send(fmt::format("加入 {} 的日期 {}", who, date),
                                conf);
                save();
            }
            else {
                conf.p->cq_send("不是一个有效日期！", conf);
            }
        }
        else if (!who.empty()) {
            conf.p->cq_send("请使用 MMDD 日期格式", conf);
        }
        else {
            conf.p->cq_send("请输入事件描述", conf);
        }
    }
    else if (command == "date.del") {
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, conf.group_id, conf.user_id)) {
            conf.p->cq_send("只有管理员可以哦", conf);
            return;
        }
        std::string who;
        getline(iss, who);
        who = trim(who);
        auto &bdays = birthdays[conf.group_id];
        auto it =
            std::remove_if(bdays.begin(), bdays.end(),
                           [&who](const mmdd &m) { return m.name == who; });
        if (it != bdays.end()) {
            bdays.erase(it, bdays.end());
            conf.p->cq_send(fmt::format("删除了 {} 的日期", who), conf);
            save();
        }
        else {
            conf.p->cq_send(fmt::format("找不到 {} 的日期", who), conf);
        }
    }
    else if (command == "date.list") {
        std::string list;
        for (const auto &b : birthdays[conf.group_id]) {
            list += fmt::format("{}: {:02d}{:02d}\n", b.name, b.mm, b.dd);
        }
        if (list.empty()) {
            list = "空空的";
        }
        conf.p->cq_send(list, conf);
    }
    else if (command == "date.send") {
        auto nowtime = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(nowtime);
        std::tm localTime = *std::localtime(&currentTime);
        send_upcoming_msg(localTime, conf.p, conf.group_id);
    } else if(command == "date.inf.add"){
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, conf.group_id, conf.user_id)) {
            conf.p->cq_send("只有管理员可以哦", conf);
            return;
        }
        int tt;
        iss >> tt;
        inform_interval.insert(tt);
        conf.p->cq_send(fmt::format("提示时长加入 {} 天", tt), conf);
    } else if(command == "date.inf.del"){
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, conf.group_id, conf.user_id)) {
            conf.p->cq_send("只有管理员可以哦", conf);
            return;
        }
        int tt;
        iss >> tt;
        inform_interval.insert(tt);
        conf.p->cq_send(fmt::format("提示时长删除 {} 天", tt), conf);
    } else if(command == "date.inf.list"){
        conf.p->cq_send(fmt::format("提示时长为 {} 天", fmt::join(inform_interval, ", ")), conf);
    } 
    else {
        conf.p->cq_send(birth_help_msg, conf);
    }
}
bool birthday::check(std::string message, const msg_meta &conf)
{
    return (message.find("date.") == 0 && conf.message_type == "group");
}
std::string birthday::help() { return "日期提醒。 date.help"; }

void birthday::send_upcoming_msg(const std::tm &localTime, bot *p,
                                 groupid_t group_idx)
{
    for (const auto &[group_id, bdays] : birthdays) {
        if (group_idx != group_id && group_idx != 0)
            continue;
        msg_meta conf = (msg_meta){"group", 0, group_id, 0, p};
        std::string todayBirthdays;
        std::string upcomingBirthdays;
        std::vector<mmdd> nearestBirthdays;
        std::vector<mmdd> nearestBirthdays1;
        std::vector<mmdd> nearestBirthdays2;
        for (const auto &b : bdays) {
            if (b.mm == (localTime.tm_mon + 1) && b.dd == localTime.tm_mday) {
                todayBirthdays += fmt::format("{}！\n", b.name);
            }
            else if (b.mm > localTime.tm_mon + 1 ||
                     (b.mm == localTime.tm_mon + 1 &&
                      b.dd > localTime.tm_mday)) {
                nearestBirthdays1.push_back(b);
            }
            else {
                nearestBirthdays2.push_back(b);
            }
        }

        std::sort(nearestBirthdays1.begin(), nearestBirthdays1.end(),
                  [](const mmdd &a, const mmdd &b) {
                      return (a.mm < b.mm) || (a.mm == b.mm && a.dd < b.dd);
                  });
        std::sort(nearestBirthdays2.begin(), nearestBirthdays2.end(),
                  [](const mmdd &a, const mmdd &b) {
                      return (a.mm < b.mm) || (a.mm == b.mm && a.dd < b.dd);
                  });

        nearestBirthdays.reserve(nearestBirthdays1.size() +
                                 nearestBirthdays2.size());
        nearestBirthdays.insert(nearestBirthdays.end(),
                                nearestBirthdays1.begin(),
                                nearestBirthdays1.end());
        nearestBirthdays.insert(nearestBirthdays.end(),
                                nearestBirthdays2.begin(),
                                nearestBirthdays2.end());

        if (!nearestBirthdays.empty()) {
            for (int i = 0; i < nearestBirthdays.size(); ++i) {
                int db = date_between(
                    (mmdd){"", localTime.tm_mon + 1, localTime.tm_mday},
                    nearestBirthdays[i], localTime.tm_year + 1900);
                if (inform_interval.find(db) != inform_interval.end()) {
                    upcomingBirthdays += fmt::format(
                        "{}: {:02d}{:02d} 还有 {} 天！\n",
                        nearestBirthdays[i].name, nearestBirthdays[i].mm,
                        nearestBirthdays[i].dd, db);
                }
            }
        }

        if (!todayBirthdays.empty()) {
            p->cq_send("今天的特殊日子！\n" + todayBirthdays, conf);
        }

        if (!upcomingBirthdays.empty()) {
            p->cq_send("接下来的日子：\n" + upcomingBirthdays, conf);
        }
    }
}

void birthday::check_date(bot *p)
{
    auto nowtime = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(nowtime);
    std::tm localTime = *std::localtime(&currentTime);
    // If now is 0 oclock,
    // find the nearest 3 birthday, store as a string: `<who> <date>\n`
    // If today is some birthday, store as `happy <who>!`
    if (localTime.tm_hour == 0) {
        if (has_sent)
            return;
        send_upcoming_msg(localTime, p);
        has_sent = true;
    }
    else {
        has_sent = false;
    }
}
birthday::~birthday() {}
void birthday::set_callback(std::function<void(std::function<void(bot *p)>)> f)
{
    f([this](bot *p) { this->check_date(p); });
}
extern "C" processable *create() { return new birthday(); }