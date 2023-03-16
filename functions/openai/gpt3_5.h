#include "processable.h"

#include <map>
#include <set>
#include <vector>
#include <jsoncpp/json/json.h>

class gpt3_5 : public processable {
private:
    std::vector<bool> is_lock;
    bool is_open, is_debug;
    std::string default_prompt;
    std::map<int64_t, std::string> pre_default;
    std::map<int64_t, Json::Value> pre_prompt;
    std::map<int64_t, Json::Value> history;
    std::vector<std::string> modes;
    std::set<int64_t> op_list;
    std::set<std::string> black_list;
    std::map<std::string, Json::Value> mode_prompt;
    std::vector<std::string> key;
    size_t key_cycle;
public:
    gpt3_5();
    size_t get_avaliable_key();
    void save_file();
    void save_history(int64_t id);
    std::string do_black(std::string u);
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};