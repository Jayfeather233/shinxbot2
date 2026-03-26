#pragma once

#include "processable.h"
#include <chrono>
#include <functional>
#include <map>
#include <string>

extern const std::string WELCOME_MESSAGE_FILE;

struct group_state_s {
    std::chrono::steady_clock::time_point last_join;
    bool timer_running = false;
    std::wstring welcome_message;
    userid_t user_id = 0;
    groupid_t group_id = 0;
    int64_t message_id = 0;
    bot *p = nullptr;
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
    void flush_welcome_queue(bot *p);

public:
    m_change_f();
    ~m_change_f();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    std::string help(const msg_meta &conf, help_level_t level);
    void
    set_callback(std::function<void(std::function<void(bot *p)>)> f) override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER