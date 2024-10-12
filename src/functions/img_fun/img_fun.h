#include "processable.h"

#include <map>

struct img_fun_type{
    enum{
        MIRROR,
        ROTATE,
        KALEIDO
    } type;
    int32_t para1: 16, para2: 16;
};

class img_fun : public processable {
private:
    std::map<userid_t, img_fun_type> is_input;
public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

DECLARE_FACTORY_FUNCTIONS_HEADER