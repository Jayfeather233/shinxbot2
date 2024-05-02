#include "informer.h"

/*
Acceptable:
hh:mm:ss
hh:mm
hh
*/
static std::string inform_help = "inform.list 列出当前消息\n"
                                 "inform.add [wday-]hh[:mm] message "
                                 "加入一条提醒消息，其中hh,mm可为*或不填默认0\n"
                                 "inform.del id 删除";

std::pair<bool, std::string> informer::isValidTime(const std::string &timeInput)
{
    // Split the input string by colons (:)
    std::vector<std::string> parts;
    std::stringstream ss(timeInput);
    std::string part;
    int wday = -1;
    if (timeInput.find('-') != timeInput.npos) {
        std::getline(ss, part, '-');
        try {
            if (part == "*") {
                wday = -1;
            }
            else {
                wday = std::stoi(part);
                if (wday <= 0 || wday >= 8) {
                    return std::make_pair(false, "");
                }
            }
        }
        catch (...) {
            return std::make_pair(false, "");
        }
    }
    while (std::getline(ss, part, ':')) {
        parts.push_back(part);
    }

    // Check the number of parts
    size_t numParts = parts.size();
    if (numParts < 1 || numParts > 2) {
        return std::make_pair(false, "");
    }

    // Define ranges for hours, minutes, and seconds
    auto isValidComponent = [](const std::string &component, int min,
                               int max) -> bool {
        if (component == "*") {
            return true;
        }
        try {
            int value = std::stoi(component);
            return value >= min && value <= max;
        }
        catch (...) {
            return false;
        }
    };

    // Validate each part
    if (numParts >= 1) {
        // Validate hours (hh)
        if (!isValidComponent(parts[0], 0, 23)) {
            return std::make_pair(false, "");
        }
    }

    if (numParts >= 2) {
        // Validate minutes (mm)
        if (!isValidComponent(parts[1], 0, 59)) {
            return std::make_pair(false, "");
        }
    }

    int i = 0;
    std::ostringstream std_reg;
    if (wday == -1) {
        std_reg << "\\d+(-)";
    }
    else {
        std_reg << wday % 7 << "(-)";
    }
    for (; i < parts.size(); ++i) {
        if (i)
            std_reg << ":";
        if (parts[i] == "*") {
            std_reg << "\\d+";
        }
        else {
            std_reg << std::setw(2) << std::setfill('0') << std::stoi(parts[i]);
        }
    }
    for (; i < 3; i++) {
        if (i)
            std_reg << ":";
        std_reg << "00";
    }

    // If all components are valid, return true
    return std::make_pair(true, std_reg.str());
}
void informer::check_inform(bot *p)
{
    auto nowtime = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(nowtime);
    std::tm localTime = *std::localtime(&currentTime);
    std::ostringstream oss;
    oss << localTime.tm_wday << "-" << std::setw(2) << std::setfill('0')
        << localTime.tm_hour << ':' << std::setw(2) << std::setfill('0')
        << localTime.tm_min << ':' << std::setw(2) << std::setfill('0')
        << localTime.tm_sec;
    std::string currentTimeString = oss.str();
    for (auto &x : inform_tuplelist) {
        for (auto &t : x.second) {
            if (std::regex_match(currentTimeString,
                                 std::regex(std::get<1>(t)))) {
                if (std::get<0>(t)) {
                    msg_meta conf;
                    if (x.first & 1) {
                        conf.message_type = "group";
                        conf.group_id = x.first >> 1;
                        conf.user_id = 0;
                    }
                    else {
                        conf.message_type = "private";
                        conf.user_id = x.first >> 1;
                        conf.group_id = 0;
                    }
                    p->cq_send(std::get<2>(t), conf);
                    std::get<0>(t) = false;
                }
            }
            else {
                std::get<0>(t) = true;
            }
        }
    }
}
std::string informer::inform_list(const msg_meta &conf)
{
    std::ostringstream oss;
    uint64_t k = conf.message_type == "private" ? (conf.user_id << 1)
                                                : ((conf.group_id << 1) + 1);
    size_t sz = this->inform_tuplelist[k].size();
    for (size_t i = 0; i < sz; ++i) {
        auto &t = this->inform_tuplelist[k][i];
        oss << "[" << i << "] Time: " << std::get<1>(t)
            << " msg: " << std::get<2>(t) << std::endl;
    }
    return oss.str();
}

void informer::process(std::string message, const msg_meta &conf)
{
    message = message.substr(7);
    if (message.find("add") == 0) {
        std::string inputtime, inputmsg;
        std::istringstream iss(message.substr(3));
        iss >> inputtime >> inputmsg;
        auto result = this->isValidTime(inputtime);
        if (result.first) {
            uint64_t k = conf.message_type == "private"
                             ? (conf.user_id << 1)
                             : ((conf.group_id << 1) + 1);
            this->inform_tuplelist[k].push_back(
                std::make_tuple(true, result.second, inputmsg));
            conf.p->cq_send("Added.", conf);
            save();
        }
        else {
            conf.p->cq_send("Invalid time pattern.", conf);
        }
    }
    else if (message.find("del") == 0) {
        size_t id = my_string2uint64(message.substr(3));
        uint64_t k = conf.message_type == "private"
                         ? (conf.user_id << 1)
                         : ((conf.group_id << 1) + 1);
        this->inform_tuplelist[k].erase(this->inform_tuplelist[k].begin() + id);
        conf.p->cq_send("Deleted id: " + std::to_string(id), conf);
        save();
    }
    else if (message.find("list") == 0) {
        conf.p->cq_send(this->inform_list(conf), conf);
    }
    else {
        conf.p->cq_send(inform_help, conf);
    }
}

void informer::save()
{
    Json::Value Ja;
    for (auto &x : this->inform_tuplelist) {
        for (auto &v : x.second) {
            Json::Value J;
            J["id"] = x.first;
            J["1"] = std::get<0>(v);
            J["2"] = std::get<1>(v);
            J["3"] = std::get<2>(v);
            Ja.append(J);
        }
    }
    writefile("./config/informer.json", Ja.toStyledString());
}

informer::informer() { read(); }
informer::~informer() { save(); }

void informer::read()
{
    Json::Value res = string_to_json(readfile("./config/informer.json", "[]"));
    for (auto u : res) {
        this->inform_tuplelist[u["id"].asUInt64()].push_back(std::make_tuple(
            u["1"].asBool(), u["2"].asString(), u["3"].asString()));
    }
}

bool informer::check(std::string message, const msg_meta &conf)
{
    return message.find("inform.") == 0;
}
std::string informer::help() { return "定时提醒。详情 inform.help"; }

void informer::set_callback(std::function<void(std::function<void(bot *p)>)> f)
{
    f([this](bot *p) { this->check_inform(p); });
}