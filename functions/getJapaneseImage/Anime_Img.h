#include "processable.h"

class AnimeImg : public processable {
public:
    AnimeImg();
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};