#include "processable.h"
#include "utils.h"

class AnimeImg : public processable {
public:
    AnimeImg();
    std::string process(std::string message, std::string message_type, long user_id, long group_id);
    bool check(std::string message, std::string message_type, long user_id, long group_id);
    std::string help();
};