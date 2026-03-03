#pragma once

#include "processable.h"
#include "utils.h"

#include <map>
#include <tuple>

class Responder : public processable {
private:
    std::map<groupid_t, std::map<std::string, std::string>> replies;
    std::map<userid_t, std::tuple<groupid_t, std::string>> is_adding;
    std::map<groupid_t, bool> trigger_by;

    void save();
    std::string get_reply_message(const std::string &message, const msg_meta &conf);
public:
    Responder();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    void set_backup_files(archivist *p, const std::string &name);
};

DECLARE_FACTORY_FUNCTIONS_HEADER