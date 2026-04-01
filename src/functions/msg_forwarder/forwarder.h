#include "processable.h"

#include <set>
#include <utility>

typedef std::pair<groupid_t, userid_t> point_t;

class forwarder : public processable {
private:
    std::set<std::pair<point_t, point_t>> forward_set;
    void load_config();

public:
    forwarder();
    size_t configure(const std::string &message, const msg_meta &conf);
    void save();
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    bool reload(const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER