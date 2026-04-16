#include "nonogram.h"
#include "utils.h"

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <random>
#include <regex>
#include <set>
#include <sstream>

static std::string get_font_path()
{
    std::array<char, 256> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen("fc-match --format=%{file} sans", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static bool is_painted_color(const Magick::ColorRGB color)
{
    return color.red() < 0.5 || color.green() < 0.5 || color.blue() < 0.5;
}

static void dfs_outer(const Magick::Image &img, int x, int y,
                      std::vector<std::vector<bool>> &is_outer, const int offx,
                      const int offy, const int max_width, const int max_height)
{
    if (x < 0 || y < 0 || x >= max_width || y >= max_height) {
        return;
    }
    if (is_outer[x][y]) {
        return;
    }
    if (!is_painted_color(img.pixelColor(x + offx, y + offy))) {
        return;
    }
    is_outer[x][y] = true;
    dfs_outer(img, x + 1, y, is_outer, offx, offy, max_width, max_height);
    dfs_outer(img, x - 1, y, is_outer, offx, offy, max_width, max_height);
    dfs_outer(img, x, y + 1, is_outer, offx, offy, max_width, max_height);
    dfs_outer(img, x, y - 1, is_outer, offx, offy, max_width, max_height);
}

static double find_threshold(const std::vector<std::vector<double>> &vv)
{
    std::vector<double> v;
    for (const auto &row : vv) {
        for (double val : row) {
            if (val >= 0.01) {
                v.push_back(std::min(val, 0.5));
            }
        }
    }

    if (v.size() < 2) {
        return 0.0;
    }

    std::sort(v.begin(), v.end());
    double c1 = v.front();
    double c2 = v.back();

    for (int iter = 0; iter < 20; ++iter) {
        double sum1 = 0, sum2 = 0;
        int cnt1 = 0, cnt2 = 0;
        for (double x : v) {
            if (std::abs(x - c1) < std::abs(x - c2)) {
                sum1 += x;
                cnt1++;
            }
            else {
                sum2 += x;
                cnt2++;
            }
        }
        if (cnt1) {
            c1 = sum1 / cnt1;
        }
        if (cnt2) {
            c2 = sum2 / cnt2;
        }
    }

    if (c1 > c2) {
        std::swap(c1, c2);
    }

    double gap = c2 - c1;
    double sse_all = 0;
    double mean = 0;
    for (double x : v) {
        mean += x;
    }
    mean /= v.size();
    for (double x : v) {
        sse_all += (x - mean) * (x - mean);
    }

    double sse1 = 0, sse2 = 0;
    size_t cnt1 = 0, cnt2 = 0;
    for (double x : v) {
        if (std::abs(x - c1) < std::abs(x - c2)) {
            sse1 += (x - c1) * (x - c1);
            cnt1++;
        }
        else {
            sse2 += (x - c2) * (x - c2);
            cnt2++;
        }
    }

    double sse_within = sse1 + sse2;
    if (c1 > 0.2 || gap < 0.3 || sse_within / sse_all > 0.9) {
        return 0.07;
    }

    double sigma1 = cnt1 ? std::sqrt(sse1 / cnt1) : 0;
    double sigma2 = cnt2 ? std::sqrt(sse2 / cnt2) : 0;
    return std::max(0.07, (c1 * sigma2 + c2 * sigma1) / (sigma1 + sigma2));
}

void nonogram::generate_puzzle_image(
    const std::vector<std::vector<int>> &row_clues,
    const std::vector<std::vector<int>> &col_clues, const std::string &filename)
{
    size_t rows = row_clues.size();
    size_t cols = col_clues.size();
    size_t max_col_clues = std::max_element(row_clues.begin(), row_clues.end(),
                                            [](const std::vector<int> &a,
                                               const std::vector<int> &b) {
                                                return a.size() < b.size();
                                            })
                               ->size();
    size_t max_row_clues = std::max_element(col_clues.begin(), col_clues.end(),
                                            [](const std::vector<int> &a,
                                               const std::vector<int> &b) {
                                                return a.size() < b.size();
                                            })
                               ->size();

    max_row_clues += 1;
    max_col_clues += 1;
    size_t img_size_row = 50 * (rows + (max_row_clues / 2 + 1)) + 50;
    size_t img_size_col = 50 * (cols + (max_col_clues / 2 + 1)) + 50;
    size_t row_header_size = 50 * (max_row_clues / 2 + 1);
    size_t col_header_size = 50 * (max_col_clues / 2 + 1);

    Magick::Image image(Magick::Geometry(img_size_col, img_size_row),
                        Magick::Color("#fdf9f2"));
    image.fillColor("black");
    image.fontFamily(get_font_path());
    image.fontPointsize(20);
    image.textAntiAlias(true);
    image.strokeAntiAlias(true);

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < row_clues[i].size(); j++) {
            image.annotate(
                std::to_string(row_clues[i][row_clues[i].size() - 1 - j]),
                Magick::Geometry(25, 50, col_header_size - 25 * (j + 1),
                                 row_header_size + 50 * i),
                Magick::GravityType::CenterGravity);
        }
    }
    for (size_t j = 0; j < cols; j++) {
        for (size_t i = 0; i < col_clues[j].size(); i++) {
            image.annotate(
                std::to_string(col_clues[j][col_clues[j].size() - 1 - i]),
                Magick::Geometry(50, 25, col_header_size + 50 * j,
                                 row_header_size - 25 * (i + 1)),
                Magick::GravityType::CenterGravity);
        }
    }

    image.strokeWidth(2);
    image.strokeColor("gray");
    for (size_t i = 0; i <= rows; i++) {
        image.draw(
            Magick::DrawableLine(col_header_size, row_header_size + 50 * i,
                                 img_size_col - 50, row_header_size + 50 * i));
    }
    for (size_t j = 0; j <= cols; j++) {
        image.draw(
            Magick::DrawableLine(col_header_size + 50 * j, row_header_size,
                                 col_header_size + 50 * j, img_size_row - 50));
    }

    image.strokeColor("black");
    for (size_t i = 0; i <= rows; i += 5) {
        image.draw(
            Magick::DrawableLine(col_header_size, row_header_size + 50 * i,
                                 img_size_col - 50, row_header_size + 50 * i));
    }
    for (size_t j = 0; j <= cols; j += 5) {
        image.draw(
            Magick::DrawableLine(col_header_size + 50 * j, row_header_size,
                                 col_header_size + 50 * j, img_size_row - 50));
    }

    image.write(filename);
}

void nonogram::generate_answer_image(std::shared_ptr<nonogram_level> level,
                                     const std::string &filename)
{
    const auto &row_clues = level->get_row_clues();
    const auto &col_clues = level->get_col_clues();
    size_t rows = row_clues.size();
    size_t cols = col_clues.size();

    size_t max_col_clues = std::max_element(row_clues.begin(), row_clues.end(),
                                            [](const std::vector<int> &a,
                                               const std::vector<int> &b) {
                                                return a.size() < b.size();
                                            })
                               ->size();
    size_t max_row_clues = std::max_element(col_clues.begin(), col_clues.end(),
                                            [](const std::vector<int> &a,
                                               const std::vector<int> &b) {
                                                return a.size() < b.size();
                                            })
                               ->size();

    max_row_clues += 1;
    max_col_clues += 1;
    size_t row_header_size = 50 * (max_row_clues / 2 + 1);
    size_t col_header_size = 50 * (max_col_clues / 2 + 1);

    generate_puzzle_image(row_clues, col_clues, filename);

    Magick::Image image;
    image.read(filename);
    image.strokeColor("black");

    const bool is_online = starts_with(level->get_pic(), "online_");
    std::mt19937 rng(std::random_device{}());
    struct hue_theme {
        double h_min;
        double h_max;
    };
    const std::vector<hue_theme> themes = {
        {0.00, 0.03}, // coral red
        {0.06, 0.12}, // amber orange
        {0.30, 0.38}, // fresh green
        {0.48, 0.55}, // cyan
        {0.56, 0.64}, // azure blue
        {0.80, 0.90}, // magenta pink
    };

    std::uniform_int_distribution<size_t> pick_theme(0, themes.size() - 1);
    const auto theme = themes[pick_theme(rng)];
    std::uniform_real_distribution<double> pick_h(theme.h_min, theme.h_max);
    std::uniform_real_distribution<double> pick_s(0.72, 0.95);
    std::uniform_real_distribution<double> pick_l(0.50, 0.68);

    const auto &data = level->get_data();
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            if (!data[i][j]) {
                continue;
            }
            if (is_online) {
                image.fillColor(
                    Magick::ColorHSL(pick_h(rng), pick_s(rng), pick_l(rng)));
            }
            else {
                image.fillColor("black");
            }
            int left = static_cast<int>(col_header_size + 50 * j + 3);
            int top = static_cast<int>(row_header_size + 50 * i + 3);
            int right = static_cast<int>(col_header_size + 50 * (j + 1) - 3);
            int bottom = static_cast<int>(row_header_size + 50 * (i + 1) - 3);
            image.draw(Magick::DrawableRectangle(left, top, right, bottom));
        }
    }
    image.write(filename);
}

void nonogram::send_puzzle(std::shared_ptr<nonogram_level> level,
                           const msg_meta &conf)
{
    std::string puzzle_path = get_cached_puzzle_path(conf);
    if (puzzle_path.empty() || !fs::exists(puzzle_path)) {
        std::string dir = ensure_nonogram_dir();
        std::string prefix = cache_key_prefix(conf) + "_" + generate_uuid();
        puzzle_path = dir + "/" + prefix + "_puzzle.png";
        std::string answer_path = dir + "/" + prefix + "_answer.png";
        generate_puzzle_image(level->get_row_clues(), level->get_col_clues(),
                              puzzle_path);
        generate_answer_image(level, answer_path);
        set_cached_paths(conf, puzzle_path, answer_path);
    }

    conf.p->cq_send("[CQ:image,file=file://" +
                        fs::absolute(fs::path(puzzle_path)).string() +
                        ",id=40000]",
                    conf);
}

double nonogram::check_cell(const Magick::Image &img, int width, int height,
                            int offx, int offy)
{
    std::vector<std::vector<bool>> is_outer(width,
                                            std::vector<bool>(height, false));

    for (int y = 0; y < width; y++) {
        dfs_outer(img, 0, y, is_outer, offx, offy, width, height);
        dfs_outer(img, width - 1, y, is_outer, offx, offy, width, height);
    }
    for (int x = 0; x < height; x++) {
        dfs_outer(img, x, 0, is_outer, offx, offy, width, height);
        dfs_outer(img, x, height - 1, is_outer, offx, offy, width, height);
    }

    size_t cnt = 0;
    size_t width_padding = width / 5;
    size_t height_padding = height / 5;
    for (size_t x = offx + width_padding; x < offx + width - width_padding;
         x++) {
        for (size_t y = offy + height_padding;
             y < offy + height - height_padding; y++) {
            if (is_painted_color(img.pixelColor(x, y))) {
                cnt += is_outer[x - offx][y - offy] ? 1 : 2;
            }
        }
    }
    return cnt * 1.0 /
           ((width - 2 * width_padding) * (height - 2 * height_padding));
}

std::vector<std::vector<double>>
nonogram::get_user_data(std::string filename,
                        std::shared_ptr<nonogram_level> level,
                        const msg_meta &conf)
{
    Magick::Image img;
    try {
        img.read(filename);
    }
    catch (...) {
        conf.p->setlog(LOG::WARNING,
                       "Failed to read submitted image: " + filename);
        conf.p->cq_send("无法读取提交的图片，请检查图片是否有效", conf);
        return std::vector<std::vector<double>>(
            level->get_row_clues().size(),
            std::vector<double>(level->get_col_clues().size(), 0));
    }

    size_t rows = level->get_row_clues().size();
    size_t cols = level->get_col_clues().size();
    size_t max_col_clues =
        std::max_element(
            level->get_row_clues().begin(), level->get_row_clues().end(),
            [](const std::vector<int> &a, const std::vector<int> &b) {
                return a.size() < b.size();
            })
            ->size();
    size_t max_row_clues =
        std::max_element(
            level->get_col_clues().begin(), level->get_col_clues().end(),
            [](const std::vector<int> &a, const std::vector<int> &b) {
                return a.size() < b.size();
            })
            ->size();
    max_row_clues += 1;
    max_col_clues += 1;
    size_t img_size_row = 50 * (rows + (max_row_clues / 2 + 1)) + 50;
    size_t img_size_col = 50 * (cols + (max_col_clues / 2 + 1)) + 50;
    size_t row_header_size = 50 * (max_row_clues / 2 + 1);
    size_t col_header_size = 50 * (max_col_clues / 2 + 1);

    img.resize(Magick::Geometry(img_size_col, img_size_row));
    img.page(Magick::Geometry(0, 0, 0, 0));

    std::vector<std::vector<double>> user_data(rows,
                                               std::vector<double>(cols, 0));
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            user_data[i][j] = check_cell(img, 50, 50, col_header_size + 50 * j,
                                         row_header_size + 50 * i);
        }
    }
    return user_data;
}

bool nonogram::check_game(std::string filename,
                          std::shared_ptr<nonogram_level> level,
                          const msg_meta &conf)
{
    auto user_data = get_user_data(filename, level, conf);
    double threshold = find_threshold(user_data);
    conf.p->setlog(LOG::INFO,
                   fmt::format("Calculated base threshold for user {}: {}",
                               conf.user_id, threshold));

    size_t rows = level->get_row_clues().size();
    size_t cols = level->get_col_clues().size();

    auto check_with_threshold = [&](double current_threshold) {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                if ((user_data[i][j] > current_threshold) !=
                    level->get_data()[i][j]) {
                    return false;
                }
            }
        }
        return true;
    };

    std::set<int> scaled_thresholds;
    std::vector<double> candidates;
    for (int delta = -8; delta <= 8; delta++) {
        double cand = threshold + delta * 0.01;
        cand = std::max(0.08, std::min(0.45, cand));
        int scaled = static_cast<int>(std::round(cand * 1000));
        if (scaled_thresholds.insert(scaled).second) {
            candidates.push_back(cand);
        }
    }

    for (double candidate : candidates) {
        if (check_with_threshold(candidate)) {
            conf.p->setlog(LOG::INFO,
                           fmt::format("Accepted threshold for user {}: {}",
                                       conf.user_id, candidate));
            return true;
        }
    }

    return false;
}

bool nonogram::handle_debug(const std::string &raw_message,
                            const msg_meta &conf)
{
    auto level = get_active_level(conf);
    if (!level) {
        conf.p->cq_send("当前没有正在进行的nonogram游戏", conf);
        return true;
    }

    std::string img = parse_guess_image(raw_message, conf);
    if (img.empty()) {
        static const std::regex debug_only_re(
            "^\\*nonogram(?:\\.debug|\\s+debug)\\s*$");
        if (std::regex_match(trim(raw_message), debug_only_re)) {
            clear_pending_guess(conf);
            mark_pending_debug(conf);
            conf.p->cq_send("请发送用于 debug 的图片", conf);
            return true;
        }
        conf.p->cq_send("debug模式：请在命令中附图，或回复一条图片消息", conf);
        return true;
    }

    clear_pending_debug(conf);

    std::string uuid = generate_uuid();
    std::string nonogram_dir = ensure_nonogram_dir();
    std::string filename = nonogram_dir + "/" + uuid + ".png";
    try {
        download(cq_decode(img), nonogram_dir + "/", uuid + ".png");
    }
    catch (...) {
        conf.p->cq_send("无法下载图片，请检查图片链接是否有效", conf);
        return true;
    }

    auto user_data = get_user_data(filename, level, conf);
    double threshold = find_threshold(user_data);
    std::ostringstream oss;
    oss << "threshold=" << fmt::format("{:.4f}", threshold) << "\n";
    for (const auto &row : user_data) {
        for (double val : row) {
            oss << (val > threshold ? '1' : '0');
        }
        oss << "\n";
    }
    conf.p->cq_send(oss.str(), conf);
    fs::remove(filename);
    return true;
}
