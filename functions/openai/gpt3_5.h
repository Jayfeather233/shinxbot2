#include "processable.h"

#include <map>
#include <set>
#include <vector>
#include <jsoncpp/json/json.h>

class gpt3_5 : public processable {
private:
    std::vector<bool> is_lock;
    bool is_open, is_debug;
    std::string close_message; // The reason for is_open=false
    std::string default_prompt;
    std::map<int64_t, std::string> pre_default;
    std::map<int64_t, Json::Value> pre_prompt;
    std::map<int64_t, Json::Value> history;
    std::vector<std::string> modes;
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
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};