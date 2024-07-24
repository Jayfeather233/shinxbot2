#include "processable.h"

#include <set>
#include <utility>

typedef std::pair<groupid_t, userid_t> point_t;

class forwarder : public processable {
private:
    std::set<std::pair<point_t, point_t> > forward_set;

public:
    forwarder();
    size_t configure(std::string messsage, const msg_meta &conf);
    void save();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

extern "C" processable* create();