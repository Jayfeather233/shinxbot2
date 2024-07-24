#include "processable.h"

class fudu : public processable {
private:
    std::map<groupid_t, std::string> gmsg;
    std::map<int64_t, int> times;

public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

extern "C" processable* create();