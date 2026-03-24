#pragma once

#include "processable.h"
#include <atomic>
#include <map>
#include <string>

extern const std::string WELCOME_MESSAGE_FILE;

struct group_state_s {
    std::chrono::steady_clock::time_point last_join;
    bool timer_running = false;
};

class m_change_f : public processable {
private:
    std::map<groupid_t, std::wstring> group_welcome_messages;
    std::map<groupid_t, group_state_s> group_state;
    std::wstring default_welcome_message;

    void save_welcome_messages();
    void load_welcome_messages();
    std::wstring get_welcome_message(groupid_t group_id);
    std::wstring format_message(const std::wstring &message,
                                const msg_meta &conf);

public:
    m_change_f();
    ~m_change_f();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    std::string help(const msg_meta &conf, help_level_t level);
};

DECLARE_FACTORY_FUNCTIONS_HEADER