#include "processable.h"

#include <jsoncpp/json/json.h>
#include <map>
#include <set>
#include <vector>
#include <mutex>

class gpt3_5 : public processable {
private:
    std::recursive_mutex data_lock;
    std::vector<bool> is_lock;
    bool is_open, is_debug;
    std::string close_message; // The reason for is_open=false
    std::string default_prompt;
    std::map<int64_t, std::string> pre_default;
    std::map<int64_t, Json::Value> pre_prompt;
    std::map<int64_t, Json::Value> history;
    std::vector<std::string> modes;
    std::set<std::string> black_list;
    std::set<int64_t> active_ids;
    std::map<std::string, Json::Value> mode_prompt;
    std::vector<std::string> key;
    size_t key_cycle;
    std::string base_url;
    std::string model_name;
    static constexpr int COMPRESS_RECENT_MESSAGES = 20;
    std::string get_quoted_content(const bot *p, int64_t reply_id, int depth = 0);
    std::string expand_forward_content(const bot *p, const std::string &forward_id, int depth);
    bool compress_history(int64_t id, size_t keyid, const msg_meta &conf,
                          std::string *error_message = nullptr);
    void fallback_trim_history(int64_t id, int rounds = 1);

    // Archive and Restore features
    void perform_archive(int64_t id, const msg_meta &conf, bool is_auto = false,
                         bool silent = false);
    uintmax_t get_archives_total_size();
    void list_archives(int64_t id, const msg_meta &conf, int page);
    void restore_archive(int64_t id, const msg_meta &conf, const std::string &arg);
    bool is_allowed_arc(int64_t id, const msg_meta &conf);

    int arc_check_counter = 0;
    bool arc_is_full = false;

public:
    gpt3_5();
    size_t get_avaliable_key();
    void save_file();
    void save_history(int64_t id);
    std::string do_black(std::string u);
    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER