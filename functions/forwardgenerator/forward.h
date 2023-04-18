#include "processable.h"
#include <jsoncpp/json/json.h>

class forward : public processable {
private:
    Json::Value get_data(std::wstring s1, std::wistringstream &wiss, int64_t group_id);
    Json::Value get_content(std::wistringstream &wiss, int64_t group_id);
public:
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};