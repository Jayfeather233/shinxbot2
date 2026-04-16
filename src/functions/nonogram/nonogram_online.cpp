#include "nonogram.h"
#include "solver.h"
#include "utils.h"

#include <cctype>
#include <regex>

static std::vector<std::string> split_keep_empty(const std::string &s,
                                                 char delim)
{
    std::vector<std::string> out;
    std::string cur;
    for (char ch : s) {
        if (ch == delim) {
            out.push_back(cur);
            cur.clear();
        }
        else {
            cur.push_back(ch);
        }
    }
    out.push_back(cur);
    return out;
}

static std::vector<int> parse_task_token(const std::string &token)
{
    std::vector<int> clue;
    static const std::regex number_re("(\\d+)");
    for (std::sregex_iterator it(token.begin(), token.end(), number_re), end;
         it != end; ++it) {
        int v = std::stoi((*it)[1].str());
        if (v > 0) {
            clue.push_back(v);
        }
    }
    return clue;
}

static std::string sanitize_puzzle_id(std::string puzzle_id)
{
    for (char &ch : puzzle_id) {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_' &&
            ch != '-') {
            ch = '_';
        }
    }
    puzzle_id = trim(puzzle_id);
    if (puzzle_id.empty()) {
        puzzle_id = generate_uuid();
    }
    return puzzle_id;
}

bool nonogram::fetch_online_level(int size,
                                  std::shared_ptr<nonogram_level> &level,
                                  std::string &puzzle_id, std::string &err_msg,
                                  bool use_proxy)
{
    try {
        std::string html =
            do_get("https://www.puzzle-nonograms.com",
                   "/?size=" + std::to_string(size), false,
                   {{"User-Agent",
                     "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36"}},
                   use_proxy);

        std::smatch m_task;
        std::smatch m_w;
        std::smatch m_h;
        std::smatch m_id;
        const std::regex task_re("\\btask\\s*=\\s*'([^']+)'");
        const std::regex width_re("\\bpuzzleWidth\\s*:\\s*(\\d+)");
        const std::regex height_re("\\bpuzzleHeight\\s*:\\s*(\\d+)");
        const std::regex id_re("id=\\\"puzzleID\\\">([^<]+)<");

        if (!std::regex_search(html, m_task, task_re) ||
            !std::regex_search(html, m_w, width_re) ||
            !std::regex_search(html, m_h, height_re) ||
            !std::regex_search(html, m_id, id_re)) {
            err_msg = "在线题库返回格式无法识别";
            return false;
        }

        int width = my_string2int64(m_w[1].str());
        int height = my_string2int64(m_h[1].str());
        if (width <= 0 || height <= 0 || width > 40 || height > 40) {
            err_msg = "在线题目尺寸异常";
            return false;
        }

        puzzle_id = sanitize_puzzle_id(m_id[1].str());
        auto parts = split_keep_empty(m_task[1].str(), '/');
        if (parts.size() != static_cast<size_t>(width + height)) {
            err_msg = "在线题目线索数量异常";
            return false;
        }

        std::vector<std::vector<int>> row_clues(height);
        std::vector<std::vector<int>> col_clues(width);
        for (int i = 0; i < height; i++) {
            row_clues[i] = parse_task_token(parts[i]);
        }
        for (int i = 0; i < width; i++) {
            col_clues[i] = parse_task_token(parts[height + i]);
        }

        std::vector<std::vector<bool>> solved;
        if (!solve_nonogram(row_clues, col_clues, solved, 1)) {
            err_msg = "在线题目求解失败（可能无解或求解器超出能力）";
            return false;
        }

        level = std::make_shared<nonogram_level>(solved, "online_" + puzzle_id +
                                                             ".png");
        return true;
    }
    catch (const std::exception &e) {
        err_msg = "在线题目抓取失败: " + std::string(e.what());
        return false;
    }
    catch (...) {
        err_msg = "在线题目抓取失败";
        return false;
    }
}
