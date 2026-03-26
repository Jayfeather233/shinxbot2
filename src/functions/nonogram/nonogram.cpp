#include "nonogram.h"
#include "utils.h"

#include <fmt/core.h>

#include <iostream>
#include <jsoncpp/json/json.h>
#include <mutex>
#include <sstream>

static std::mutex data_rw;

static std::string nonogram_help_msg =
    "nonogram游戏，输入 *nonogram [1~3] 开始游戏\n"
    "目前有3个难度，分别为：\n"
    "1：10x10\n"
    "2：15x15\n"
    "3：20x20";

void nonogram::load()
{
    Json::Value J;
    try {
        J = string_to_json(readfile(
            bot_config_path(nullptr, "features/nonogram/nonogram.json"), "[]"));
    }
    catch (...) {
        set_global_log(LOG::WARNING, "Failed to load nonogram levels");
        return;
    }
    for (const Json::Value &level : J) {
        std::vector<std::vector<bool>> data;
        for (const Json::Value &row : level["data"]) {
            std::vector<bool> row_data;
            for (const Json::Value &cell : row) {
                row_data.push_back(cell.asBool());
            }
            data.push_back(row_data);
        }
        levels.emplace_back(data, level["pic"].asString());
    }
}

nonogram::nonogram()
{
    std::lock_guard<std::mutex> lock(data_rw);
    load();
}

void nonogram::reload()
{
    std::lock_guard<std::mutex> lock(data_rw);
    user_current_levels.clear();
    group_current_levels.clear();
    levels.clear();
    load();
}

std::string nonogram::parse_image_url(std::string message)
{
    size_t img_pos = message.find("[CQ:image");
    if (img_pos != std::string::npos) {
        size_t url_pos = message.find(",url=");
        size_t url_end_pos = message.find("]", url_pos);
        if (url_pos != std::string::npos && url_end_pos != std::string::npos) {
            return message.substr(url_pos + 5, url_end_pos - (url_pos + 5));
        }
    }
    return "";
}

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

void nonogram::generate_puzzle_image(
    const std::vector<std::vector<int>> &row_clues,
    const std::vector<std::vector<int>> &col_clues, const std::string &filename)
{
    size_t rows = row_clues.size();
    size_t cols = col_clues.size();
    size_t max_col_clues =
        std::max_element(
            row_clues.begin(), row_clues.end(),
            [](const std::vector<int> &a, const std::vector<int> &b) {
                return a.size() < b.size(); // 返回 true 表示 a 比 b “小”
            })
            ->size();
    size_t max_row_clues =
        std::max_element(
            col_clues.begin(), col_clues.end(),
            [](const std::vector<int> &a, const std::vector<int> &b) {
                return a.size() < b.size(); // 返回 true 表示 a 比 b “小”
            })
            ->size();
    max_row_clues += 1;
    max_col_clues += 1;
    size_t img_size_row =
        50 * (rows + (max_row_clues / 2 + 1)) + 50; // +50 for padding
    size_t img_size_col = 50 * (cols + (max_col_clues / 2 + 1)) + 50;
    size_t row_header_size = 50 * (max_row_clues / 2 + 1);
    size_t col_header_size = 50 * (max_col_clues / 2 + 1);
    Magick::Image image(Magick::Geometry(img_size_col, img_size_row),
                        Magick::Color("#fdf9f2"));

    // Draw clues
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

    // Draw grid inner lines
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

    // Draw grid bold lines every 5 lines
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

void nonogram::send_puzzle(std::shared_ptr<nonogram_level> level,
                           const msg_meta &conf)
{
    std::string uuid = generate_uuid();
    const std::string puzzle_path =
        bot_resource_path(nullptr, "nonogram/" + uuid + ".png");
    generate_puzzle_image(level->get_row_clues(), level->get_col_clues(),
                          puzzle_path);
    conf.p->cq_send("[CQ:image,file=file://" +
                        fs::absolute(fs::path(puzzle_path)).string() +
                        ",id=40000]",
                    conf);
    fs::remove(puzzle_path);
}

inline bool is_painted_color(const Magick::ColorRGB color)
{
    return color.red() < 0.5 || color.green() < 0.5 || color.blue() < 0.5;
}

void dfs_outer(const Magick::Image &img, int x, int y,
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

double nonogram::check_cell(const Magick::Image &img, int width, int height,
                            int offx, int offy)
{
    std::vector<std::vector<bool>> is_outer =
        std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));
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

double find_threshold(const std::vector<std::vector<double>> &vv)
{
    std::vector<double> v;
    for (const auto &row : vv) {
        for (double val : row) {
            if (val >= 0.01) {
                if (val >= 0.5)
                    val = 0.5;
                v.push_back(val);
            }
        }
    }

    if (v.size() < 2)
        return 0.0;

    // 1. 排序
    std::sort(v.begin(), v.end());

    // 2. 初始化两个中心（最小 & 最大）
    double c1 = v.front();
    double c2 = v.back();

    // 3. k-means 迭代
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

        if (cnt1)
            c1 = sum1 / cnt1;
        if (cnt2)
            c2 = sum2 / cnt2;
    }

    // 4. 保证 c1 < c2
    if (c1 > c2)
        std::swap(c1, c2);

    // 5. 判断是否真的分成两块
    double gap = c2 - c1;

    double sse_all = 0;
    double mean = 0;
    for (double x : v)
        mean += x;
    mean /= v.size();
    for (double x : v)
        sse_all += (x - mean) * (x - mean);

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
    // fmt::print("c1: {}, c2: {}, gap: {}, sse1: {}, sse2: {}, sse_all: {}\n",
    // c1, c2, gap, sse1, sse2, sse_all);
    double sse_within = sse1 + sse2;
    if (c1 > 0.2 || gap < 0.3 || sse_within / sse_all > 0.9) {
        return 0.07; // 无法有效区分，返回默认值
    }
    double sigma1 = cnt1 ? std::sqrt(sse1 / cnt1) : 0;
    double sigma2 = cnt2 ? std::sqrt(sse2 / cnt2) : 0;
    return std::max(0.07, (c1 * sigma2 + c2 * sigma1) / (sigma1 + sigma2));
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
                return a.size() < b.size(); // 返回 true 表示 a 比 b “小”
            })
            ->size();
    size_t max_row_clues =
        std::max_element(
            level->get_col_clues().begin(), level->get_col_clues().end(),
            [](const std::vector<int> &a, const std::vector<int> &b) {
                return a.size() < b.size(); // 返回 true 表示 a 比 b “小”
            })
            ->size();
    max_row_clues += 1;
    max_col_clues += 1;
    size_t img_size_row =
        50 * (rows + (max_row_clues / 2 + 1)) + 50; // +50 for padding
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
                   fmt::format("Calculated threshold for user {}: {}",
                               conf.user_id, threshold));

    size_t rows = level->get_row_clues().size();
    size_t cols = level->get_col_clues().size();
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            if ((user_data[i][j] > threshold) != level->get_data()[i][j]) {
                return false;
            }
        }
    }
    return true;
}

void nonogram::process_command(std::string message, const msg_meta &conf)
{
    if (message == "*nonogram.help") {
        conf.p->cq_send(nonogram_help_msg, conf);
        return;
    }
    if (message == "*nonogram.reload") {
        if (!conf.p->is_op(conf.user_id)) {
            return;
        }
        reload();
        conf.p->cq_send("关卡已重新加载", conf);
        return;
    }
    if (message == "*nonogram.quit") {
        if (conf.message_type == "group") {
            group_current_levels.erase(conf.group_id);
        }
        else if (conf.message_type == "private") {
            user_current_levels.erase(conf.user_id);
        }
        conf.p->cq_send("游戏已退出", conf);
        return;
    }
    if (message.find("*nonogram") == 0) {
        std::string index_str = trim(message.substr(9));
        int index = 0;
        if (index_str.empty()) {
            index = get_random(levels.size());
        }
        else {
            index = my_string2int64(index_str) - 1;
        }

        if (index < 0 || index >= levels.size()) {
            conf.p->cq_send("无效的关卡索引", conf);
            return;
        }
        if (levels.empty()) {
            conf.p->cq_send("当前没有可用的关卡", conf);
            return;
        }
        std::shared_ptr<nonogram_level> level =
            std::make_shared<nonogram_level>(levels[index]);
        if (conf.message_type == "group") {
            if (group_current_levels.find(conf.group_id) !=
                group_current_levels.end()) {
                send_puzzle(group_current_levels[conf.group_id], conf);
                conf.p->cq_send("本群已经有一个正在进行的nonogram游戏了，请完成"
                                "它或者等待它结束后再开始新的游戏",
                                conf);
            }
            else {
                group_current_levels[conf.group_id] = level;
                send_puzzle(group_current_levels[conf.group_id], conf);
                conf.p->cq_send("@bot并回复图片以提交答案", conf);
            }
        }
        else if (conf.message_type == "private") {
            if (user_current_levels.find(conf.user_id) !=
                user_current_levels.end()) {
                send_puzzle(user_current_levels[conf.user_id], conf);
                conf.p->cq_send("你已经有一个正在进行的nonogram游戏了，请完成它"
                                "或者等待它结束后再开始新的游戏",
                                conf);
            }
            else {
                user_current_levels[conf.user_id] = level;
                send_puzzle(user_current_levels[conf.user_id], conf);
                conf.p->cq_send("@bot并回复图片以提交答案", conf);
            }
        }
    }
}

void nonogram::process(std::string message, const msg_meta &conf)
{
    if (message.find("*nonogram") == 0) {
        process_command(message, conf);
        return;
    }
    if (message.find("[CQ:at") == std::string::npos ||
        message.find("qq=" + std::to_string(conf.p->get_botqq())) ==
            std::string::npos) {
        return;
    }
    std::string img;
    if (message.find("[CQ:reply") != std::string::npos) {
        size_t pos = message.find("[CQ:reply,id=");
        size_t end_pos = message.find("]", pos);
        if (pos != std::string::npos && end_pos != std::string::npos) {
            std::string reply_id_str =
                message.substr(pos + 13, end_pos - (pos + 13));
            int64_t reply_id = my_string2int64(reply_id_str);
            Json::Value J;
            J["message_id"] = reply_id;
            std::string res = conf.p->cq_send("get_msg", J);
            Json::Value res_json = string_to_json(res);
            if (res_json.isMember("data") &&
                res_json["data"].isMember("message")) {
                std::string reply_message =
                    messageArr_to_string(res_json["data"]["message"]);
                img = parse_image_url(reply_message);
            }
        }
        else {
            conf.p->setlog(LOG::WARNING, "Failed to parse reply message id");
            return;
        }
    }
    else {
        img = parse_image_url(message);
    }
    if (img.empty()) {
        conf.p->setlog(LOG::WARNING, "No image found in the message");
        return;
    }
    std::string uuid = generate_uuid();
    const std::string nonogram_dir = bot_resource_path(nullptr, "nonogram");
    std::string filename = nonogram_dir + "/" + uuid + ".png";
    try {
        download(cq_decode(img), nonogram_dir + "/", uuid + ".png");
    }
    catch (...) {
        conf.p->setlog(LOG::WARNING, "Failed to download image: " + img);
        conf.p->cq_send("无法下载图片，请检查图片链接是否有效", conf);
        return;
    }
    bool result = false;
    std::lock_guard<std::mutex> lock(data_rw);
    if (conf.message_type == "group" &&
        group_current_levels.find(conf.group_id) !=
            group_current_levels.end()) {

        if (message.find("test") != std::string::npos) {
            auto user_data = get_user_data(
                filename, group_current_levels[conf.group_id], conf);
            std::ostringstream oss;
            for (const auto &row : user_data) {
                for (double val : row) {
                    oss << fmt::format("{:.2f} ", val);
                }
                oss << "\n";
            }
            conf.p->cq_send("用户数据：\n" + oss.str(), conf);
            fs::remove(filename);
            return;
        }
        result =
            check_game(filename, group_current_levels[conf.group_id], conf);

        if (result) {
            conf.p->cq_send(
                fmt::format("恭喜你，答案正确！\n[CQ:image,file=file://{}/"
                            "resource/nonogram/answer/{},id=40000]",
                            get_local_path(),
                            group_current_levels[conf.group_id]->get_pic()),
                conf);
            group_current_levels.erase(conf.group_id);
        }
        else {
            conf.p->cq_send("答案不正确，请继续努力！", conf);
        }
    }
    else if (conf.message_type == "private" &&
             user_current_levels.find(conf.user_id) !=
                 user_current_levels.end()) {

        if (message.find("test") != std::string::npos) {
            auto user_data = get_user_data(
                filename, user_current_levels[conf.user_id], conf);
            std::ostringstream oss;
            for (const auto &row : user_data) {
                for (double val : row) {
                    oss << fmt::format("{:.2f} ", val);
                }
                oss << "\n";
            }
            conf.p->cq_send("用户数据：\n" + oss.str(), conf);
            fs::remove(filename);
            return;
        }
        result = check_game(filename, user_current_levels[conf.user_id], conf);

        if (result) {
            conf.p->cq_send(
                fmt::format("恭喜你，答案正确！\n[CQ:image,file=file://{}/"
                            "resource/nonogram/answer/{},id=40000]",
                            get_local_path(),
                            user_current_levels[conf.user_id]->get_pic()),
                conf);
            user_current_levels.erase(conf.user_id);
        }
        else {
            conf.p->cq_send("答案不正确，请继续努力！", conf);
        }
    }
    fs::remove(filename);
}
bool nonogram::check(std::string message, const msg_meta &conf)
{
    if (message.find("*nonogram") == 0) {
        return true;
    }
    if (conf.message_type == "group" &&
        group_current_levels.find(conf.group_id) !=
            group_current_levels.end()) {
        return true;
    }
    if (conf.message_type == "private" &&
        user_current_levels.find(conf.user_id) != user_current_levels.end()) {
        return true;
    }
    return false;
}
std::string nonogram::help()
{
    return "nonogram 游戏：输入 *nonogram [1~3] 开始。帮助：*nonogram.help";
}

DECLARE_FACTORY_FUNCTIONS(nonogram)