#include "utils.h"

#include <iostream>
#include <jsoncpp/json/json.h>

Json::Value string_to_json(const std::string &str)
{
    Json::Value root;
    Json::Reader re;
    bool isok = re.parse(str, root);
    if (!isok) {
        set_global_log(LOG::ERROR,
               "string to json failed: " + re.getFormattedErrorMessages() +
                   "\nstring: " + str);
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