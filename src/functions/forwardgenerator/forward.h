#include "processable.h"
#include <jsoncpp/json/json.h>

class forward_msg_gen : public processable {
private:
    Json::Value get_data(bot *p, std::wstring s1, std::wistringstream &wiss,
                         int64_t group_id);
    Json::Value get_content(bot *p, std::wistringstream &wiss, int64_t group_id);

public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
