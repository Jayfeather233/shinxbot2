#pragma once

#include "processable.h"
#include "utils.h"
#include <set>
#include <mutex>

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
inline int date_between(const mmdd &a, const mmdd &b, const int year)
{
    if (a.mm > b.mm || (a.mm == b.mm && a.dd > b.dd)) {
        return date_between(a, (mmdd){"", 12, 31}, year) +
               date_between((mmdd){"", 1, 1}, b, year + 1) + 1;
    }
    else {
        int ret = 0;
        for (int i = a.mm; i < b.mm; i++) {
            ret += max_mmday[i - 1];
            if (i == 2 &&
                !(year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))) {
                ret--;
            }
        }
        ret += b.dd - a.dd;
        return ret;
    }
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
    std::set<int> inform_interval;
    std::map<groupid_t, std::vector<mmdd>> birthdays;
    std::mutex mutex_;
    bool has_sent = true;
    void send_upcoming_msg(const std::tm &localTime, bot *p, groupid_t group_idx = 0);

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
DECLARE_FACTORY_FUNCTIONS_HEADER