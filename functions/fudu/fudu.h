#include "processable.h"

class fudu : public processable {
private:
    std::map<int64_t, std::string> gmsg;
    std::map<int64_t, int> times;

public:
    void process(shinx_message msg);
    bool check(shinx_message msg);
    std::string help();
};
