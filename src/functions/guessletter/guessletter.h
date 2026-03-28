#pragma once

#include "processable.h"

#include <chrono>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class guessletter : public processable {
private:
    struct bank_entry {
        std::string scope;
        std::string cn_name;
        std::string en_name;
        std::string pinyin_key;
        std::string english_key;
        std::vector<std::string> aliases;
    };

    struct question {
        std::string answer;
        std::string cn_answer;
        std::string key;
        std::string shown;
        std::vector<std::string> accepted_answers;
        bool solved = false;
        userid_t solved_by = 0;
    };

    struct session {
        bool started = false;
        bool free_mode = false;
        userid_t host = 0;
        std::vector<userid_t> players;
        int round_idx = 0;
        int question_count = 5;
        std::string range = "pinyin";
        std::string scope = "StrawberryJam";
        std::vector<question> questions;
        std::unordered_map<userid_t, int> scores;
        std::set<char> opened_letters;
        std::unordered_map<userid_t, std::chrono::steady_clock::time_point>
            last_open_at;
    };

    std::vector<bank_entry> bank_;
    std::unordered_map<groupid_t, session> sessions_;
    std::mutex lock_;

    const std::string bank_path_ =
        bot_config_path("features/guessletter/letterbank.json");

    bool load_bank();
    static std::string norm_key(const std::string &s);
    static std::string norm_answer(const std::string &s);
    static bool is_ascii_alpha_num(char c);
    bool is_admin(const msg_meta &conf) const;

    session &get_or_create_session(groupid_t gid, userid_t host);
    bool in_session(const session &s, userid_t uid) const;
    std::string render_question_line(int idx, const question &q,
                                     const msg_meta &conf) const;
    std::string render_board(const session &s, const msg_meta &conf) const;
    std::string render_scoreboard(const session &s, const msg_meta &conf) const;
    std::string render_opened_letters(const session &s) const;
    std::vector<question> build_questions(const session &s) const;
    void advance_turn(session &s);
    bool all_solved(const session &s) const;

public:
    guessletter();
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER
