#include "processable.h"

#include <map>
#include <set>
#include <jsoncpp/json/json.h>

class gpt3_5 : public processable {
private:
    bool is_lock, is_open;
    Json::Value default_prompt;
    std::map<int64_t, std::string> pre_default;
    std::map<int64_t, Json::Value> pre_prompt;
    std::map<int64_t, Json::Value> history;
    std::set<std::string> modes;
    std::set<int64_t> op_list;
    std::set<std::string> black_list;
    std::map<std::string, Json::Value> mode_prompt;
    std::string key;
public:
    gpt3_5();
    std::string do_black(std::string u);
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};