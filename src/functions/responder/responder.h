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

    void load();
    void save();
    std::string get_reply_message(const std::string &message,
                                  const msg_meta &conf);
    void send_reply_by_trigger(groupid_t group_id, const std::string &trigger,
                               const msg_meta &conf);

public:
    Responder();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    bool reload(const msg_meta &conf) override;
    std::string help();
    void set_backup_files(archivist *p, const std::string &name);
};

DECLARE_FACTORY_FUNCTIONS_HEADER