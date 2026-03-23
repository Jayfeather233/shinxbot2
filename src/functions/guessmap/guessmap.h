#pragma once

#include "processable.h"
#include <Magick++.h>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class guessmap : public processable {
private:
    struct reveal_box {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        int crop_size = 256;
    };

    struct map_entry {
        std::string file_path;
        std::string answer;
        std::unordered_set<std::wstring> aliases;
        std::vector<std::string> images;
    };

    struct session_state {
        bool active = false;
        size_t map_index = 0;
        std::string source_image;
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        int crop_size = 256;
        int total_guess_count = 0;
        int roll_count = 0;
        int hint_count = 0;
        int image_seq = 0;
        int current_track = 0;
        std::chrono::steady_clock::time_point started_at;
        std::unordered_set<long long> guessed_users;
        std::vector<reveal_box> tracks;
        std::vector<std::string> hint_images;
        std::vector<std::string> temp_files;
    };

    std::vector<map_entry> maps_;
    std::unordered_map<std::string, session_state> sessions_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        cooldown_;
    std::unordered_map<std::string, std::deque<size_t>> recent_map_history_;
    std::mutex lock_;
    std::string maps_json_path_;
    std::string images_root_path_;

    static constexpr int kEasyCrop = 256;
    static constexpr int kHardCrop = 128;
    static constexpr int kUltraCrop = 64;
    static constexpr int kCooldownSec = 45;
    static constexpr int kHintExpandStep = 32;
    static constexpr int kMaxRollCount = 4;
    static constexpr int kMaxHintCount = 6;
    static constexpr size_t kRecentMapHistory = 10;

    std::string get_scope_id(const msg_meta &conf) const;
    std::wstring normalize(const std::string &text) const;
    bool is_start_cmd(const std::string &message) const;
    int get_start_crop(const std::string &message) const;
    std::string get_guess_arg(const std::string &message) const;
    bool is_cooldown_active(const std::string &id) const;
    std::string hint_output_path(const std::string &id, int seq) const;

    bool load_maps();
    size_t pick_map_index(const std::string &id) const;
    void record_recent_map(const std::string &id, size_t map_index);
    bool start_game(const std::string &id, int crop_size);
    bool is_nonsense(const Magick::Image &img) const;
    std::string cropped_output_path(const std::string &id) const;
    std::string reveal_output_path(const std::string &id) const;
    bool write_hint_crop(const session_state &state, int left, int top,
                         int crop_size, const std::string &out_file) const;
    bool try_random_position(const session_state &state, int crop_size,
                             int &left, int &top) const;
    bool append_hint_image(session_state &state, const std::string &id) const;
    bool roll_hint(session_state &state, const std::string &id);
    bool expand_hint(session_state &state, const std::string &id);
    bool build_reveal_image(const session_state &state,
                            const std::string &out_file) const;
    void cleanup_generated_images(const std::string &id) const;
    void send_all_hint_images(const msg_meta &conf, const session_state &state,
                              const std::string &text) const;

    void react_or_reply(const msg_meta &conf, const std::string &emoji_id,
                        const std::string &fallback_text) const;
    void send_guess_prompt(const msg_meta &conf,
                           const std::string &crop_file) const;
    void send_finish_message(const msg_meta &conf, const std::string &text,
                             const std::string &image_file) const;

public:
    guessmap();

    void process(std::string message, const msg_meta &conf) override;
    bool check(std::string message, const msg_meta &conf) override;
    std::string help() override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER
