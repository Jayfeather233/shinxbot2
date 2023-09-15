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
