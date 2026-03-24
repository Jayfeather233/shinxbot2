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

    void process_command(std::string message, const msg_meta &conf);
    std::string parse_image_url(std::string message);
    bool check_game(std::string filename, std::shared_ptr<nonogram_level> level,
                    const msg_meta &conf);
    void send_puzzle(std::shared_ptr<nonogram_level> level,
                     const msg_meta &conf);
    void generate_puzzle_image(const std::vector<std::vector<int>> &row_clues,
                               const std::vector<std::vector<int>> &col_clues,
                               const std::string &filename);
    double check_cell(const Magick::Image &img, int width, int height, int offx,
                      int offy);
    std::vector<std::vector<double>>
    get_user_data(std::string filename, std::shared_ptr<nonogram_level> level,
                  const msg_meta &conf);
    void reload();
    void load();

public:
    nonogram();
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

DECLARE_FACTORY_FUNCTIONS_HEADER