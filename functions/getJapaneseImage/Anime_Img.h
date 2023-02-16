#include "processable.h"

class AnimeImg : public processable {
public:
    AnimeImg();
    void process(std::string message, std::string message_type, long user_id, long group_id);
    bool check(std::string message, std::string message_type, long user_id, long group_id);
    std::string help();
};