#include "processable.h"
#include <jsoncpp/json/json.h>

class forward_msg_gen : public processable {
private:
    Json::Value get_data(bot *p, std::wstring s1, std::wistringstream &wiss,
                         uint64_t group_id);
    Json::Value get_content(bot *p, std::wistringstream &wiss, uint64_t group_id);

    Json::Value get_data(Json::Value message, const msg_meta &conf);
    Json::Value get_content(Json::Value message, const msg_meta &conf);
public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    bool is_support_messageArr();
    // void process(Json::Value message, const msg_meta &conf);
    // bool check(Json::Value message, const msg_meta &conf);
    std::string help();
};

extern "C" processable* create();