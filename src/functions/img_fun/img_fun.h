#include "processable.h"

#include <map>

struct img_fun_type{
    enum{
        MIRROR,
        ROTATE,
        KALEIDO
    } type;
    int para1;
    int para2;
};

class img_fun : public processable {
private:
    std::map<int64_t, img_fun_type> is_input;
public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
