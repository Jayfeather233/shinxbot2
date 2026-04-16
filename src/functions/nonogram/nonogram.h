#pragma once

#include "processable.h"

#include <Magick++.h>

#include <map>
#include <memory>
#include <vector>

class nonogram_level {
    std::vector<std::vector<bool>> data;
    std::vector<std::vector<int>> row_clues;
    std::vector<std::vector<int>> col_clues;
    std::string pic;

public:
    nonogram_level(const std::vector<std::vector<bool>> &d,
                   const std::string &p)
    {
        data = d;
        pic = p;
        int rows = data.size();
        int cols = data[0].size();
        row_clues.resize(rows);
        col_clues.resize(cols);
        for (int i = 0; i < rows; i++) {
            int count = 0;
            for (int j = 0; j < cols; j++) {
                if (data[i][j]) {
                    count++;
                }
                else {
                    if (count > 0) {
                        row_clues[i].push_back(count);
                        count = 0;
                    }
                }
            }
            if (count > 0) {
                row_clues[i].push_back(count);
            }
        }
        for (int j = 0; j < cols; j++) {
            int count = 0;
            for (int i = 0; i < rows; i++) {
                if (data[i][j]) {
                    count++;
                }
                else {
                    if (count > 0) {
                        col_clues[j].push_back(count);
                        count = 0;
                    }
                }
            }
            if (count > 0) {
                col_clues[j].push_back(count);
            }
        }
    }
    const std::vector<std::vector<int>> &get_row_clues() const
    {
        return row_clues;
    }
    const std::vector<std::vector<int>> &get_col_clues() const
    {
        return col_clues;
    }
    const std::vector<std::vector<bool>> &get_data() const { return data; }
    const std::string &get_pic() const { return pic; }
};

class nonogram : public processable {
private:
    std::vector<nonogram_level> levels;
    std::map<userid_t, std::shared_ptr<nonogram_level>> user_current_levels;
    std::map<groupid_t, std::shared_ptr<nonogram_level>> group_current_levels;
    std::map<userid_t, std::string> user_puzzle_cache;
    std::map<groupid_t, std::string> group_puzzle_cache;
    std::map<userid_t, std::string> user_answer_cache;
    std::map<groupid_t, std::string> group_answer_cache;
    std::map<userid_t, bool> pending_guess_users;
    std::map<userid_t, bool> pending_debug_users;

    void process_command(std::string message, const msg_meta &conf);
    bool start_level_for_session(std::shared_ptr<nonogram_level> level,
                                 const msg_meta &conf,
                                 const std::string &loaded_msg = "");
    bool handle_local_start(const std::string &args, const msg_meta &conf);
    bool handle_online_start(const std::string &args, const msg_meta &conf);
    bool handle_guess(const std::string &raw_message, const msg_meta &conf);
    bool handle_debug(const std::string &raw_message, const msg_meta &conf);
    bool handle_giveup(const msg_meta &conf);
    std::string parse_image_url(std::string message);
    std::string parse_guess_image(std::string message, const msg_meta &conf);
    std::string ensure_nonogram_dir();
    std::string cache_key_prefix(const msg_meta &conf);
    std::string get_cached_puzzle_path(const msg_meta &conf);
    std::string get_cached_answer_path(const msg_meta &conf);
    void set_cached_paths(const msg_meta &conf, const std::string &puzzle_path,
                          const std::string &answer_path);
    void clear_cached_paths(const msg_meta &conf);
    void cleanup_orphan_cache_files();
    bool has_pending_guess(const msg_meta &conf) const;
    void mark_pending_guess(const msg_meta &conf);
    void clear_pending_guess(const msg_meta &conf);
    bool has_pending_debug(const msg_meta &conf) const;
    void mark_pending_debug(const msg_meta &conf);
    void clear_pending_debug(const msg_meta &conf);
    bool has_active_game(const msg_meta &conf) const;
    std::shared_ptr<nonogram_level> get_active_level(const msg_meta &conf);
    void set_active_level(const msg_meta &conf,
                          std::shared_ptr<nonogram_level> level);
    void clear_active_level(const msg_meta &conf);
    bool check_game(std::string filename, std::shared_ptr<nonogram_level> level,
                    const msg_meta &conf);
    void send_puzzle(std::shared_ptr<nonogram_level> level,
                     const msg_meta &conf);
    void generate_puzzle_image(const std::vector<std::vector<int>> &row_clues,
                               const std::vector<std::vector<int>> &col_clues,
                               const std::string &filename);
    void generate_answer_image(std::shared_ptr<nonogram_level> level,
                               const std::string &filename);
    double check_cell(const Magick::Image &img, int width, int height, int offx,
                      int offy);
    std::vector<std::vector<double>>
    get_user_data(std::string filename, std::shared_ptr<nonogram_level> level,
                  const msg_meta &conf);
    bool fetch_online_level(int size, std::shared_ptr<nonogram_level> &level,
                            std::string &puzzle_id, std::string &err_msg,
                            bool use_proxy = false);
    bool send_win_message(std::shared_ptr<nonogram_level> level,
                          const msg_meta &conf);
    void reload();
    void load();

public:
    nonogram();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    bool reload(const msg_meta &conf) override;
    std::string help() override;
    std::string help(const msg_meta &conf,
                     help_level_t level = help_level_t::public_only) override;
};

DECLARE_FACTORY_FUNCTIONS_HEADER