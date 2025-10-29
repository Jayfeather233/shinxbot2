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
            set_global_log(LOG::ERROR,
                "string to json failed: " + re.getFormattedErrorMessages() +
                    "\nstring: " + str);
        }
    } catch (std::exception &e) {
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