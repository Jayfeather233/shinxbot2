#include "processable.h"

#include <map>

struct img_fun_type {
    enum { MIRROR, ROTATE, KALEIDO } type;
    int32_t para1 : 16, para2 : 16;
};

class img_fun : public processable {
private:
    std::map<userid_t, img_fun_type> is_input;

public:
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER