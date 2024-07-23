#pragma once

#include "processable.h"
#include "utils.h"

struct mmdd {
    std::string name;
    int mm;
    int dd;
};

static int max_mmday[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
inline bool check_valid_date(const mmdd &u)
{
    return (0 < u.mm && u.mm < 13 && 0 < u.dd && u.dd <= max_mmday[u.mm - 1]);
}

/*
{
"group_id":
[
{"who": name, "mm": mm, "dd", dd}
]
}

*/
class birthday : public processable {
private:
    std::map<uint64_t, std::vector<mmdd>> birthdays;
    bool has_sent = true;

public:
    birthday();
    void save();

    /*
    birthday.add mmdd who
    birthday.del who
    */
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    ~birthday();
    void check_date(bot *p);
    void set_callback(std::function<void(std::function<void(bot *p)>)> f);
};

extern "C" processable *create();