#include "processable.h"

#include <regex>

/*
Acceptable:
hh:mm:ss
hh:mm
hh
*/

class informer : public processable {
private:
    // user/group_id -> [is_informed, reg pattern, msg]
    std::map<uint64_t, std::vector<std::tuple<bool, std::string, std::string>>> inform_tuplelist;

    std::pair<bool, std::string> isValidTime(const std::string &timeInput);
    void check_inform(bot *p);
    std::string inform_list(const msg_meta &conf);

    void save();
    void read();
public:
    informer();
    ~informer();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();

    void set_callback(std::function<void(std::function<void(bot *p)>)> f);
};