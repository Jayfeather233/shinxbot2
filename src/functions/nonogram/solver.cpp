#include "solver.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

namespace {

struct col_state {
    int clue_idx = 0;
    int run_len = 0;
    bool in_run = false;
};

void generate_row_patterns_rec(const std::vector<int> &clues, int width,
                               int clue_pos, int cursor,
                               std::vector<int> &current,
                               std::vector<std::vector<int>> &out)
{
    if (clue_pos >= static_cast<int>(clues.size())) {
        current.resize(width, 0);
        out.push_back(current);
        return;
    }

    int block = clues[clue_pos];
    int remain_blocks = 0;
    for (int i = clue_pos + 1; i < static_cast<int>(clues.size()); i++) {
        remain_blocks += clues[i];
    }
    int remain_gaps =
        std::max(0, static_cast<int>(clues.size()) - clue_pos - 1);
    int min_needed_after = remain_blocks + remain_gaps;

    for (int start = cursor; start + block + min_needed_after <= width;
         start++) {
        std::vector<int> next = current;
        if (static_cast<int>(next.size()) < start) {
            next.resize(start, 0);
        }
        for (int i = static_cast<int>(next.size()); i < start; i++) {
            next.push_back(0);
        }
        for (int i = 0; i < block; i++) {
            next.push_back(1);
        }

        if (clue_pos + 1 < static_cast<int>(clues.size())) {
            next.push_back(0);
            generate_row_patterns_rec(clues, width, clue_pos + 1,
                                      static_cast<int>(next.size()), next, out);
        }
        else {
            next.resize(width, 0);
            out.push_back(next);
        }
    }
}

std::vector<std::vector<int>>
generate_row_patterns(const std::vector<int> &clues, int width)
{
    std::vector<std::vector<int>> out;
    std::vector<int> current;
    generate_row_patterns_rec(clues, width, 0, 0, current, out);
    if (clues.empty()) {
        out.push_back(std::vector<int>(width, 0));
    }
    return out;
}

bool apply_cell(const std::vector<int> &col_clue, col_state &st, int cell)
{
    if (cell) {
        if (st.clue_idx >= static_cast<int>(col_clue.size())) {
            return false;
        }
        if (!st.in_run) {
            st.in_run = true;
            st.run_len = 1;
        }
        else {
            st.run_len++;
        }
        if (st.run_len > col_clue[st.clue_idx]) {
            return false;
        }
    }
    else {
        if (st.in_run) {
            if (st.run_len != col_clue[st.clue_idx]) {
                return false;
            }
            st.in_run = false;
            st.run_len = 0;
            st.clue_idx++;
        }
    }
    return true;
}

bool column_feasible(const std::vector<int> &col_clue, const col_state &st,
                     int rows_left)
{
    int need = 0;
    if (st.in_run) {
        if (st.clue_idx >= static_cast<int>(col_clue.size())) {
            return false;
        }
        int cur_remaining = col_clue[st.clue_idx] - st.run_len;
        if (cur_remaining < 0) {
            return false;
        }
        need += cur_remaining;

        int rest_blocks = 0;
        for (int i = st.clue_idx + 1; i < static_cast<int>(col_clue.size());
             i++) {
            rest_blocks += col_clue[i];
        }
        int rest_count = static_cast<int>(col_clue.size()) - st.clue_idx - 1;
        need += rest_blocks;
        if (rest_count > 0) {
            need += rest_count;
        }
    }
    else {
        int rest_blocks = 0;
        for (int i = st.clue_idx; i < static_cast<int>(col_clue.size()); i++) {
            rest_blocks += col_clue[i];
        }
        int rest_count = static_cast<int>(col_clue.size()) - st.clue_idx;
        need += rest_blocks;
        if (rest_count > 0) {
            need += rest_count - 1;
        }
    }
    return need <= rows_left;
}

bool finalized_ok(const std::vector<int> &col_clue, col_state st)
{
    if (!apply_cell(col_clue, st, 0)) {
        return false;
    }
    return st.clue_idx == static_cast<int>(col_clue.size()) && !st.in_run;
}

} // namespace

bool solve_nonogram(const std::vector<std::vector<int>> &row_clues,
                    const std::vector<std::vector<int>> &col_clues,
                    std::vector<std::vector<bool>> &out_grid, int max_solutions)
{
    int h = static_cast<int>(row_clues.size());
    int w = static_cast<int>(col_clues.size());
    if (h <= 0 || w <= 0) {
        return false;
    }

    std::vector<std::vector<std::vector<int>>> row_patterns(h);
    for (int r = 0; r < h; r++) {
        row_patterns[r] = generate_row_patterns(row_clues[r], w);
        if (row_patterns[r].empty()) {
            return false;
        }
    }

    std::vector<std::vector<bool>> board(h, std::vector<bool>(w, false));
    std::vector<col_state> states(w);
    int solutions = 0;
    std::vector<std::vector<bool>> first_solution;

    std::function<void(int)> dfs = [&](int r) {
        if (solutions >= std::max(1, max_solutions)) {
            return;
        }
        if (r == h) {
            for (int c = 0; c < w; c++) {
                if (!finalized_ok(col_clues[c], states[c])) {
                    return;
                }
            }
            solutions++;
            if (first_solution.empty()) {
                first_solution = board;
            }
            return;
        }

        int rows_left_after = h - r - 1;
        for (const auto &pattern : row_patterns[r]) {
            auto backup = states;
            bool ok = true;

            for (int c = 0; c < w; c++) {
                if (!apply_cell(col_clues[c], states[c], pattern[c])) {
                    ok = false;
                    break;
                }
            }

            if (ok) {
                for (int c = 0; c < w; c++) {
                    if (!column_feasible(col_clues[c], states[c],
                                         rows_left_after)) {
                        ok = false;
                        break;
                    }
                }
            }

            if (ok) {
                for (int c = 0; c < w; c++) {
                    board[r][c] = (pattern[c] != 0);
                }
                dfs(r + 1);
            }
            states = backup;
            if (solutions >= std::max(1, max_solutions)) {
                return;
            }
        }
    };

    dfs(0);
    if (solutions <= 0 || first_solution.empty()) {
        return false;
    }
    out_grid = std::move(first_solution);
    return true;
}
