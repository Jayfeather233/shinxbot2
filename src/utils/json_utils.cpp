#include "utils.h"

#include <iostream>
#include <jsoncpp/json/json.h>

Json::Value string_to_json(const std::string &str)
{
    Json::Value root;
    Json::Reader re;
    try {
        bool isok = re.parse(str, root);
        if (!isok) {
            set_global_log(LOG::ERROR, "string to json failed: " +
                                           re.getFormattedErrorMessages() +
                                           "\nstring: " + str);
        }
    }
    catch (std::exception &e) {
        set_global_log(LOG::ERROR,
                       "string to json exception: " + std::string(e.what()) +
                           "\nstring: " + str);
        throw "Json parse error";
    }
    return root;
}

void parse_json_to_set(const Json::Value &J, std::set<uint64_t> &mp)
{
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        mp.insert(J[i].asUInt64());
    }
}
void parse_json_to_set(const Json::Value &J, std::set<std::string> &mp)
{
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        mp.insert(J[i].asString());
    }
}

Json::ArrayIndex json_array_find(const Json::Value &J, const uint64_t &data)
{
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        try {
            if (J[i].asUInt64() == data) {
                return i;
            }
        }
        catch (...) { // Json::LogicError
            continue;
        }
    }
    return sz;
}

// message format like:
// xx[CQ:at,qq=123456]xx[CQ:image,file=xxx.jpg]xx
Json::Value expand_string_to_messageArr(std::string s) {
    Json::Value messageArr;
    size_t pos = 0;
    if ((pos = s.find("[CQ:")) == std::string::npos) {
        Json::Value jj;
        jj["type"] = "text";
        jj["data"]["text"] = s;
        messageArr.append(jj);
        return messageArr;
    } else {
        size_t last_pos = 0;
        while (pos != std::string::npos) {
            if (pos > last_pos) {
                Json::Value jj;
                jj["type"] = "text";
                jj["data"]["text"] = s.substr(last_pos, pos - last_pos);
                messageArr.append(jj);
            }
            size_t end_pos = s.find("]", pos);
            if (end_pos == std::string::npos) {
                break;
            }
            std::string cq_code = s.substr(pos + 4, end_pos - pos - 4);
            size_t comma_pos = cq_code.find(",");
            Json::Value jj;
            jj["type"] = cq_code.substr(0, comma_pos);
            while (comma_pos < cq_code.size()) {
                size_t next_comma = cq_code.find(",", comma_pos + 1);
                if (next_comma == std::string::npos) {
                    next_comma = cq_code.size();
                }
                std::string item = cq_code.substr(comma_pos + 1, next_comma - comma_pos - 1);
                size_t equal_pos = item.find("=");
                if (equal_pos != std::string::npos) {
                    std::string key = item.substr(0, equal_pos);
                    std::string value = item.substr(equal_pos + 1);
                    jj["data"][key] = cq_decode(value);
                } else {
                    jj["data"]["0"] = item;
                }
                comma_pos = next_comma;
            }
            messageArr.append(jj);
            last_pos = end_pos + 1;
            pos = s.find("[CQ:", last_pos);
        }
        if (last_pos < s.size()) {
            Json::Value jj;
            jj["type"] = "text";
            jj["data"]["text"] = s.substr(last_pos);
            messageArr.append(jj);
        }
        return messageArr;
    }
}