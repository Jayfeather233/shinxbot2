#pragma once

#include <vector>

// Solve a nonogram from row/column clues.
// Returns true and writes one solution into out_grid when solvable.
// If max_solutions >= 2, solver may continue briefly to check ambiguity.
bool solve_nonogram(const std::vector<std::vector<int>> &row_clues,
                    const std::vector<std::vector<int>> &col_clues,
                    std::vector<std::vector<bool>> &out_grid,
                    int max_solutions = 1);
