#include "forwarder.h"
#include "utils.h"

#include <iostream>

inline bool check_valid(const point_t &a, const point_t &b)
{
    return (a.first == -1 || b.first == -1 || a.first == b.first) &&
           (a.second == -1 || b.second == -1 || a.second == b.second);
}

forwarder::forwarder()
{
    Json::Value J = string_to_json(readfile("./config/forwarder.json", "[]"));
    for (Json::Value j : J) {
        point_t from, to;
        from = std::make_pair<int64_t, int64_t>(j["from"]["group_id"].asInt64(),
                                                j["from"]["user_id"].asInt64());
        to = std::make_pair<int64_t, int64_t>(j["to"]["group_id"].asInt64(),
                                              j["to"]["user_id"].asInt64());
        forward_set.insert(std::make_pair(from, to));
    }
}

void forwarder::save()
{
    Json::Value Ja;
    for (auto it : forward_set) {
        Json::Value J;
        J["from"]["group_id"] = it.first.first;
        J["from"]["user_id"] = it.first.second;
        J["to"]["group_id"] = it.second.first;
        J["to"]["user_id"] = it.second.second;
        Ja.append(J);
    }
    writefile("./config/forwarder.json", Ja.toStyledString());
}

void forwarder::configure(std::string message)
{
    if (message.find("forward.set") == 0) {
        std::istringstream iss(message.substr(11));
        point_t from, to;
        iss >> from.first >> from.second >> to.first >> to.second;
        forward_set.insert(std::make_pair(from, to));
    } else if(message.find("forward.del") == 0){
        std::istringstream iss(message.substr(11));
        point_t from, to;
        iss >> from.first >> from.second >> to.first >> to.second;
        forward_set.erase(std::make_pair(from, to));
    }
    save();
}

void forwarder::process(std::string message, const msg_meta &conf)
{
    if (is_op(conf.user_id) && message.find("forward.") == 0) {
        configure(message);
        cq_send("done.", conf);
        return;
    }
    point_t fr = std::make_pair(conf.group_id, conf.user_id);
    for(auto it : forward_set){
        if(it.first == fr){
            if(it.second.first == -1){
                cq_send(message, (msg_meta){"private", it.second.second, it.second.first, 0});
            } else {
                cq_send(message, (msg_meta){"group", -1, it.second.first, 0});
            }
        }
    }
}
bool forwarder::check(std::string message, const msg_meta &conf)
{
    return true;
}
std::string forwarder::help() { return ""; }