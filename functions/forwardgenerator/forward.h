#include "processable.h"
#include <jsoncpp/json/json.h>

class forward : public processable {
private:
    Json::Value get_data(std::wstring s1, std::wistringstream &wiss, long group_id);
    Json::Value get_content(std::wistringstream &wiss, long group_id);
public:
    void process(std::string message, std::string message_type, long user_id, long group_id);
    bool check(std::string message, std::string message_type, long user_id, long group_id);
    std::string help();
};