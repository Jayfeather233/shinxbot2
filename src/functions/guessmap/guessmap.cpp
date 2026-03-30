#include "guessmap.h"

#include "utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

struct crop_region {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
};

static crop_region get_center_16_9_region(int w, int h)
{
    crop_region r;
    if (w <= 0 || h <= 0) {
        return r;
    }

    const double target = 16.0 / 9.0;
    const double ar = static_cast<double>(w) / static_cast<double>(h);

    if (ar > target) {
        r.height = h;
        r.width = static_cast<int>(std::round(h * target));
        r.left = (w - r.width) / 2;
        r.top = 0;
    }
    else {
        r.width = w;
        r.height = static_cast<int>(std::round(w / target));
        r.left = 0;
        r.top = (h - r.height) / 2;
    }

    r.width = std::max(0, std::min(r.width, w));
    r.height = std::max(0, std::min(r.height, h));
    r.left = std::max(0, std::min(r.left, w - r.width));
    r.top = std::max(0, std::min(r.top, h - r.height));
    return r;
}

static bool is_image_file(const fs::path &p)
{
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
}

static void erase_first_path(std::vector<std::string> &paths,
                             const std::string &target)
{
    auto it = std::find(paths.begin(), paths.end(), target);
    if (it != paths.end()) {
        paths.erase(it);
    }
}

static std::string guessmap_detail_help()
{
    return "蔚蓝猜地图\n"
           "*guess.help: 查看本帮助\n"
           "*guess_start_easy/hard/ultra/imp: 开始猜图\n"
           "*guess_start_任意文本: 随机难度开始（16-256）\n"
           "*guess <答案>: 提交答案\n"
           "*guess_roll: 同尺寸换位置\n"
           "*guess_hint: 扩大提示范围\n"
           "*guess_cd: 查看当前每次提示需要猜测次数\n"
           "*guess_cd 数字: 设置当前提示冷却\n"
           "*guess_giveup: 放弃并揭晓答案\n"
           "*guess_check: 检查题库状态";
}

static std::pair<std::string, std::string>
parse_collection_and_hall(const std::string &file_path)
{
    std::string normalized = file_path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    std::vector<std::string> parts;
    std::stringstream ss(normalized);
    std::string part;
    while (std::getline(ss, part, '/')) {
        part = trim(part);
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    const std::string collection =
        parts.empty() ? std::string("(unknown)") : parts[0];
    const std::string hall =
        parts.size() >= 2 ? parts[1] : std::string("(未分大厅)");
    return {collection, hall};
}

guessmap::guessmap()
{
    fs::create_directories(cache_root_dir());
    load_cooldown_config();
    if (!load_maps()) {
        set_global_log(LOG::WARNING, "guessmap: failed to load maps from " +
                                         maps_config_path());
    }
}

std::string guessmap::maps_config_path() const
{
    return (fs::path(config_dir_) / "features/guessmap/maps.json").string();
}

std::string guessmap::images_root_dir() const
{
    return (fs::path(resource_dir_) / "guessmap/images").string();
}

std::string guessmap::cache_root_dir() const
{
    return (fs::path(resource_dir_) / "guessmap").string();
}

std::string guessmap::cooldown_config_path() const
{
    return (fs::path(config_dir_) / "features/guessmap/cooldown.json").string();
}

void guessmap::sync_dirs_from_bot(const bot *p)
{
    if (p == nullptr) {
        return;
    }
    const std::string new_cfg = p->getConfigDir();
    const std::string new_res = p->getResourceDir();
    if (new_cfg == config_dir_ && new_res == resource_dir_) {
        return;
    }
    config_dir_ = new_cfg;
    resource_dir_ = new_res;
    fs::create_directories(cache_root_dir());
    load_cooldown_config();
}

void guessmap::load_cooldown_config()
{
    assist_cooldown_guess_by_scope_.clear();
    Json::Value root = string_to_json(readfile(cooldown_config_path(), "{}"));
    if (!root.isObject()) {
        return;
    }

    if (root.isMember("default_guess_cooldown") &&
        root["default_guess_cooldown"].isInt()) {
        int v = root["default_guess_cooldown"].asInt();
        if (v < 0) {
            v = 0;
        }
        assist_cooldown_guess_by_scope_["__default__"] = v;
    }

    if (root.isMember("scope") && root["scope"].isObject()) {
        for (const auto &k : root["scope"].getMemberNames()) {
            int v = root["scope"][k].asInt();
            assist_cooldown_guess_by_scope_[k] = std::max(0, v);
        }
    }
}

void guessmap::save_cooldown_config() const
{
    Json::Value root(Json::objectValue);
    auto it_default = assist_cooldown_guess_by_scope_.find("__default__");
    root["default_guess_cooldown"] =
        (it_default == assist_cooldown_guess_by_scope_.end())
            ? kDefaultAssistGuessCooldown
            : it_default->second;

    Json::Value scope(Json::objectValue);
    for (const auto &kv : assist_cooldown_guess_by_scope_) {
        if (kv.first == "__default__") {
            continue;
        }
        scope[kv.first] = kv.second;
    }
    root["scope"] = scope;
    writefile(cooldown_config_path(), root.toStyledString(), false);
}

int guessmap::get_assist_cooldown_guess(const std::string &id) const
{
    auto it = assist_cooldown_guess_by_scope_.find(id);
    if (it != assist_cooldown_guess_by_scope_.end()) {
        return std::max(0, it->second);
    }
    auto it_default = assist_cooldown_guess_by_scope_.find("__default__");
    if (it_default != assist_cooldown_guess_by_scope_.end()) {
        return std::max(0, it_default->second);
    }
    return kDefaultAssistGuessCooldown;
}

int guessmap::available_assist_uses(const session_state &state,
                                    int cooldown_guess) const
{
    if (cooldown_guess <= 0) {
        return 1;
    }
    const int earned = state.total_guess_count / cooldown_guess;
    return earned - state.assist_used_count;
}

bool guessmap::can_use_assist(const session_state &state,
                              int cooldown_guess) const
{
    return available_assist_uses(state, cooldown_guess) > 0;
}

std::string guessmap::get_scope_id(const msg_meta &conf) const
{
    if (conf.message_type == "group") {
        return "g" + std::to_string(conf.group_id);
    }
    return "u" + std::to_string(conf.user_id);
}

std::wstring guessmap::normalize(const std::string &text) const
{
    std::wstring w = string_to_wstring(text);
    std::wstring out;
    out.reserve(w.size());

    for (wchar_t c : w) {
        wchar_t lc = std::towlower(c);
        if (lc == L' ' || lc == L'.' || lc == L',' || lc == L'-' ||
            lc == L'\'' || lc == L'!' || lc == L'，' || lc == L'！' ||
            lc == L'…' || lc == L'。' || lc == L':' || lc == L'：' ||
            lc == L'+' || lc == L'_' || lc == L'\n' || lc == L'\r' ||
            lc == L'\t') {
            continue;
        }
        out.push_back(lc);
    }
    return out;
}

bool guessmap::is_start_cmd(const std::string &message) const
{
    return cmd_match_prefix(message, {"*guess_start"});
}

int guessmap::get_start_crop(const std::string &message) const
{
    if (cmd_match_prefix(message, {"*guess_start_hard"})) {
        return kHardCrop;
    }
    if (cmd_match_prefix(message, {"*guess_start_ultra"})) {
        return kUltraCrop;
    }
    if (cmd_match_prefix(message, {"*guess_start_imp"})) {
        return get_random(8, 17);
    }
    std::string body;
    if (cmd_strip_prefix(message, "*guess_start_", body)) {
        std::string mode = body;
        size_t split = mode.find_first_of(" \t");
        if (split != std::string::npos) {
            mode = mode.substr(0, split);
        }
        std::transform(
            mode.begin(), mode.end(), mode.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!mode.empty() && mode != "easy" && mode != "hard" &&
            mode != "ultra" && mode != "imp") {
            return get_random(16, 257);
        }
    }
    return kEasyCrop;
}

std::string guessmap::get_guess_arg(const std::string &message) const
{
    std::string arg;
    if (!cmd_strip_prefix(message, "*guess", arg)) {
        return "";
    }
    return arg;
}

bool guessmap::is_cooldown_active(const std::string &id) const
{
    auto it = cooldown_.find(id);
    if (it == cooldown_.end()) {
        return false;
    }
    return std::chrono::steady_clock::now() < it->second;
}

size_t guessmap::pick_map_index(const std::string &id) const
{
    std::vector<size_t> all_candidates;
    all_candidates.reserve(maps_.size());

    std::unordered_set<size_t> recent;
    auto it = recent_map_history_.find(id);
    if (it != recent_map_history_.end()) {
        recent.insert(it->second.begin(), it->second.end());
    }

    std::vector<size_t> fresh_candidates;
    fresh_candidates.reserve(maps_.size());

    for (size_t i = 0; i < maps_.size(); ++i) {
        if (maps_[i].images.empty()) {
            continue;
        }
        all_candidates.push_back(i);
        if (recent.find(i) == recent.end()) {
            fresh_candidates.push_back(i);
        }
    }

    const std::vector<size_t> &pool =
        fresh_candidates.empty() ? all_candidates : fresh_candidates;
    if (pool.empty()) {
        return maps_.size();
    }
    return pool[static_cast<size_t>(get_random(static_cast<int>(pool.size())))];
}

void guessmap::record_recent_map(const std::string &id, size_t map_index)
{
    std::deque<size_t> &history = recent_map_history_[id];
    history.erase(std::remove(history.begin(), history.end(), map_index),
                  history.end());
    history.push_back(map_index);
    while (history.size() > kRecentMapHistory) {
        history.pop_front();
    }
}

bool guessmap::load_maps()
{
    std::string maps_json = maps_config_path();
    std::string images_root = images_root_dir();

    Json::Value root = string_to_json(readfile(maps_json, "[]"));
    if (!root.isArray()) {
        return false;
    }

    std::vector<map_entry> loaded;
    loaded.reserve(root.size());

    for (const auto &item : root) {
        if (!item.isMember("file_path") || !item.isMember("answer")) {
            continue;
        }

        map_entry e;
        e.file_path = trim(item["file_path"].asString());
        e.answer = trim(item["answer"].asString());

        e.aliases.insert(normalize(e.answer));
        if (item.isMember("alias") && item["alias"].isArray()) {
            for (const auto &a : item["alias"]) {
                e.aliases.insert(normalize(a.asString()));
            }
        }

        fs::path dir = fs::path(images_root) / e.file_path;
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            continue;
        }

        for (const auto &entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (!is_image_file(entry.path())) {
                continue;
            }
            e.images.push_back(entry.path().string());
        }

        if (!e.images.empty()) {
            loaded.push_back(std::move(e));
        }
    }

    maps_ = std::move(loaded);
    maps_json_path_ = maps_json;
    images_root_path_ = images_root;
    return !maps_.empty();
}

bool guessmap::is_nonsense(const Magick::Image &img) const
{
    const size_t w = img.columns();
    const size_t h = img.rows();
    if (w == 0 || h == 0) {
        return true;
    }

    double rs = 0.0, gs = 0.0, bs = 0.0;
    double r2 = 0.0, g2 = 0.0, b2 = 0.0;
    const double n = static_cast<double>(w * h);

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            Magick::ColorRGB c(img.pixelColor(x, y));
            const double r = c.red() * 255.0;
            const double g = c.green() * 255.0;
            const double b = c.blue() * 255.0;
            rs += r;
            gs += g;
            bs += b;
            r2 += r * r;
            g2 += g * g;
            b2 += b * b;
        }
    }

    const double rv = r2 / n - (rs / n) * (rs / n);
    const double gv = g2 / n - (gs / n) * (gs / n);
    const double bv = b2 / n - (bs / n) * (bs / n);

    return rv + gv + bv < 300.0;
}

std::string guessmap::cropped_output_path(const std::string &id) const
{
    fs::path p = fs::path(cache_root_dir()) / (id + "_crop.png");
    return fs::absolute(p).string();
}

std::string guessmap::reveal_output_path(const std::string &id) const
{
    fs::path p = fs::path(cache_root_dir()) / (id + "_reveal.png");
    return fs::absolute(p).string();
}

std::string guessmap::hint_output_path(const std::string &id, int seq) const
{
    fs::path p = fs::path(cache_root_dir()) /
                 (id + "_hint_" + std::to_string(seq) + ".png");
    return fs::absolute(p).string();
}

bool guessmap::write_hint_crop(const session_state &state, int left, int top,
                               int crop_size, const std::string &out_file) const
{
    Magick::Image img;
    try {
        img.read(state.source_image);
        img.crop(Magick::Geometry(crop_size, crop_size, left, top));
        img.page(Magick::Geometry(0, 0, 0, 0));
        img.write(out_file);
    }
    catch (...) {
        return false;
    }
    return true;
}

bool guessmap::try_random_position(const session_state &state, int crop_size,
                                   int &left, int &top) const
{
    Magick::Image img;
    try {
        img.read(state.source_image);
    }
    catch (...) {
        return false;
    }

    const int w = static_cast<int>(img.columns());
    const int h = static_cast<int>(img.rows());
    const crop_region playable = get_center_16_9_region(w, h);
    if (playable.width < crop_size || playable.height < crop_size) {
        return false;
    }

    Magick::Image cropped;
    bool ok = false;
    for (int i = 0; i < 20; ++i) {
        left = get_random(playable.left,
                          playable.left + playable.width - crop_size + 1);
        top = get_random(playable.top,
                         playable.top + playable.height - crop_size + 1);
        cropped = img;
        cropped.crop(Magick::Geometry(crop_size, crop_size, left, top));
        cropped.page(Magick::Geometry(0, 0, 0, 0));
        if (!is_nonsense(cropped)) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        left = get_random(playable.left,
                          playable.left + playable.width - crop_size + 1);
        top = get_random(playable.top,
                         playable.top + playable.height - crop_size + 1);
    }
    return true;
}

bool guessmap::append_hint_image(session_state &state,
                                 const std::string &id) const
{
    const std::string out = hint_output_path(id, state.image_seq++);
    if (!write_hint_crop(state, state.left, state.top, state.crop_size, out)) {
        return false;
    }
    state.hint_images.push_back(out);
    state.temp_files.push_back(out);
    return true;
}

bool guessmap::roll_hint(session_state &state, const std::string &id,
                         bool ignore_usage_cap)
{
    if (!ignore_usage_cap && state.roll_count >= kMaxRollCount) {
        return false;
    }

    int left = state.left;
    int top = state.top;
    if (!try_random_position(state, state.crop_size, left, top)) {
        return false;
    }

    state.left = left;
    state.top = top;
    state.right = left + state.crop_size;
    state.bottom = top + state.crop_size;
    state.roll_count += 1;

    reveal_box box;
    box.left = state.left;
    box.top = state.top;
    box.right = state.right;
    box.bottom = state.bottom;
    box.crop_size = state.crop_size;
    state.tracks.push_back(box);
    state.current_track = static_cast<int>(state.tracks.size()) - 1;

    return append_hint_image(state, id);
}

bool guessmap::expand_hint(session_state &state, const std::string &id,
                           bool ignore_usage_cap)
{
    if (!ignore_usage_cap && state.hint_count >= kMaxHintCount) {
        return false;
    }

    Magick::Image img;
    try {
        img.read(state.source_image);
    }
    catch (...) {
        return false;
    }

    const int w = static_cast<int>(img.columns());
    const int h = static_cast<int>(img.rows());
    const crop_region playable = get_center_16_9_region(w, h);
    if (playable.width <= 0 || playable.height <= 0) {
        return false;
    }

    int new_size = state.crop_size + kHintExpandStep;
    new_size = std::min(new_size, playable.width);
    new_size = std::min(new_size, playable.height);
    if (new_size <= state.crop_size) {
        return false;
    }

    const int cx = state.left + state.crop_size / 2;
    const int cy = state.top + state.crop_size / 2;
    int new_left = cx - new_size / 2;
    int new_top = cy - new_size / 2;
    new_left =
        std::max(playable.left,
                 std::min(new_left, playable.left + playable.width - new_size));
    new_top =
        std::max(playable.top,
                 std::min(new_top, playable.top + playable.height - new_size));

    state.crop_size = new_size;
    state.left = new_left;
    state.top = new_top;
    state.right = state.left + state.crop_size;
    state.bottom = state.top + state.crop_size;
    state.hint_count += 1;

    if (state.current_track >= 0 &&
        state.current_track < static_cast<int>(state.tracks.size())) {
        state.tracks[state.current_track].left = state.left;
        state.tracks[state.current_track].top = state.top;
        state.tracks[state.current_track].right = state.right;
        state.tracks[state.current_track].bottom = state.bottom;
        state.tracks[state.current_track].crop_size = state.crop_size;
    }

    const std::string out = hint_output_path(id, state.image_seq++);
    if (!write_hint_crop(state, state.left, state.top, state.crop_size, out)) {
        return false;
    }

    if (state.current_track >= 0 &&
        state.current_track < static_cast<int>(state.hint_images.size())) {
        const std::string old = state.hint_images[state.current_track];
        std::error_code ec;
        fs::remove(old, ec);
        erase_first_path(state.temp_files, old);
        state.hint_images[state.current_track] = out;
    }
    else {
        state.hint_images.push_back(out);
    }
    state.temp_files.push_back(out);
    return true;
}

bool guessmap::start_game(const std::string &id, int crop_size)
{
    if (maps_.empty()) {
        load_maps();
    }
    if (maps_.empty()) {
        return false;
    }

    for (int attempt = 0; attempt < 100; ++attempt) {
        size_t mi = pick_map_index(id);
        if (mi >= maps_.size()) {
            return false;
        }
        const auto &m = maps_[mi];
        if (m.images.empty()) {
            continue;
        }
        size_t ii =
            static_cast<size_t>(get_random(static_cast<int>(m.images.size())));
        const std::string &img_file = m.images[ii];

        Magick::Image img;
        try {
            img.read(img_file);
        }
        catch (...) {
            continue;
        }

        const int w = static_cast<int>(img.columns());
        const int h = static_cast<int>(img.rows());
        const crop_region playable = get_center_16_9_region(w, h);
        if (playable.width < crop_size || playable.height < crop_size) {
            continue;
        }

        int left = 0;
        int top = 0;
        Magick::Image cropped;
        bool ok = false;
        for (int i = 0; i < 20; ++i) {
            left = get_random(playable.left,
                              playable.left + playable.width - crop_size + 1);
            top = get_random(playable.top,
                             playable.top + playable.height - crop_size + 1);
            cropped = img;
            cropped.crop(Magick::Geometry(crop_size, crop_size, left, top));
            cropped.page(Magick::Geometry(0, 0, 0, 0));
            if (!is_nonsense(cropped)) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            cropped = img;
            cropped.crop(Magick::Geometry(crop_size, crop_size, left, top));
            cropped.page(Magick::Geometry(0, 0, 0, 0));
        }

        session_state st;
        st.active = true;
        st.map_index = mi;
        st.source_image = img_file;
        st.left = left;
        st.top = top;
        st.right = left + crop_size;
        st.bottom = top + crop_size;
        st.crop_size = crop_size;
        st.total_guess_count = 0;
        st.roll_count = 0;
        st.hint_count = 0;
        st.assist_used_count = 0;
        st.image_seq = 0;
        st.current_track = 0;
        st.started_at = std::chrono::steady_clock::now();
        st.guessed_users.clear();

        reveal_box box;
        box.left = st.left;
        box.top = st.top;
        box.right = st.right;
        box.bottom = st.bottom;
        box.crop_size = st.crop_size;
        st.tracks.push_back(box);

        if (!append_hint_image(st, id)) {
            continue;
        }

        sessions_[id] = st;
        record_recent_map(id, mi);
        cooldown_[id] = std::chrono::steady_clock::now() +
                        std::chrono::seconds(kCooldownSec);
        return true;
    }

    return false;
}

bool guessmap::build_reveal_image(const session_state &state,
                                  const std::string &out_file) const
{
    Magick::Image img;
    try {
        img.read(state.source_image);
    }
    catch (...) {
        return false;
    }

    const int w = static_cast<int>(img.columns());
    const int h = static_cast<int>(img.rows());
    const crop_region playable = get_center_16_9_region(w, h);
    if (playable.width <= 0 || playable.height <= 0) {
        return false;
    }

    try {
        img.crop(Magick::Geometry(playable.width, playable.height,
                                  playable.left, playable.top));
        img.page(Magick::Geometry(0, 0, 0, 0));
    }
    catch (...) {
        return false;
    }

    Magick::ColorRGB red(1.0, 0.0, 0.0);
    const int cw = static_cast<int>(img.columns());
    const int ch = static_cast<int>(img.rows());

    const std::vector<reveal_box> &boxes =
        state.tracks.empty()
            ? std::vector<reveal_box>{reveal_box{state.left, state.top,
                                                 state.right, state.bottom,
                                                 state.crop_size}}
            : state.tracks;

    for (const auto &box_src : boxes) {
        const int rel_left = box_src.left - playable.left;
        const int rel_top = box_src.top - playable.top;
        const int rel_right = box_src.right - playable.left;
        const int rel_bottom = box_src.bottom - playable.top;

        const int l = std::max(0, std::min(rel_left, cw - 1));
        const int t = std::max(0, std::min(rel_top, ch - 1));
        const int r = std::max(0, std::min(rel_right, cw));
        const int b = std::max(0, std::min(rel_bottom, ch));

        for (int thick = 0; thick < 7; ++thick) {
            int y1 = t + thick;
            int y2 = b - 1 - thick;
            int x1 = l + thick;
            int x2 = r - 1 - thick;
            if (y1 >= 0 && y1 < ch) {
                for (int x = l; x < r; ++x) {
                    if (x >= 0 && x < cw) {
                        img.pixelColor(x, y1, red);
                    }
                }
            }
            if (y2 >= 0 && y2 < ch) {
                for (int x = l; x < r; ++x) {
                    if (x >= 0 && x < cw) {
                        img.pixelColor(x, y2, red);
                    }
                }
            }
            if (x1 >= 0 && x1 < cw) {
                for (int y = t; y < b; ++y) {
                    if (y >= 0 && y < ch) {
                        img.pixelColor(x1, y, red);
                    }
                }
            }
            if (x2 >= 0 && x2 < cw) {
                for (int y = t; y < b; ++y) {
                    if (y >= 0 && y < ch) {
                        img.pixelColor(x2, y, red);
                    }
                }
            }
        }
    }

    try {
        img.write(out_file);
    }
    catch (...) {
        return false;
    }
    return true;
}

void guessmap::cleanup_generated_images(const std::string &id) const
{
    auto it = sessions_.find(id);
    if (it != sessions_.end()) {
        std::error_code ec;
        for (const auto &f : it->second.temp_files) {
            fs::remove(f, ec);
            ec.clear();
        }
    }

    std::error_code ec;
    fs::remove(cropped_output_path(id), ec);
    ec.clear();
    fs::remove(reveal_output_path(id), ec);
}

void guessmap::send_all_hint_images(const msg_meta &conf,
                                    const session_state &state,
                                    const std::string &text) const
{
    std::string msg = text;
    bool first = true;
    for (const auto &f : state.hint_images) {
        if (first) {
            msg += "\n";
            first = false;
        }
        msg += "[CQ:image,file=file://" + f + ",id=40000]";
    }
    conf.p->cq_send(msg, conf);
}

void guessmap::react_or_reply(const msg_meta &conf, const std::string &emoji_id,
                              const std::string &fallback_text) const
{
    bool ok = false;
    try {
        Json::Value J;
        J["message_id"] = conf.message_id;
        J["emoji_id"] = emoji_id;
        Json::Value R =
            string_to_json(conf.p->cq_send("set_msg_emoji_like", J));
        ok = R["status"].asString() == "ok";
    }
    catch (...) {
    }

    if (!ok && !fallback_text.empty()) {
        conf.p->cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                            "]" + fallback_text,
                        conf);
    }
}

void guessmap::send_guess_prompt(const msg_meta &conf,
                                 const std::string &crop_file) const
{
    conf.p->cq_send(
        "[CQ:image,file=file://" + crop_file +
            ",id=40000]这个截图是出自哪张图呢？\n输入*guess 你的答案 以回答",
        conf);
}

void guessmap::send_finish_message(const msg_meta &conf,
                                   const std::string &text,
                                   const std::string &image_file) const
{
    conf.p->cq_send(text + "[CQ:image,file=file://" + image_file + ",id=40000]",
                    conf);
}

bool guessmap::check(std::string message, const msg_meta &conf)
{
    (void)conf;
    return cmd_match_prefix(trim(message), {"*guess"});
}

bool guessmap::reload(const msg_meta &conf)
{
    std::lock_guard<std::mutex> guard(lock_);
    sync_dirs_from_bot(conf.p);
    sessions_.clear();
    cooldown_.clear();
    recent_map_history_.clear();
    load_cooldown_config();
    return load_maps();
}

void guessmap::process(std::string message, const msg_meta &conf)
{
    message = trim(message);
    const std::string id = get_scope_id(conf);

    std::lock_guard<std::mutex> guard(lock_);
    sync_dirs_from_bot(conf.p);

    auto handle_guess_cd = [&]() {
        const int cd = get_assist_cooldown_guess(id);
        if (cd <= 0) {
            conf.p->cq_send("当前提示冷却: 0（roll/hint 不受猜测次数限制）",
                            conf);
        }
        else {
            conf.p->cq_send("当前提示冷却: 每猜 " + std::to_string(cd) +
                                " 次可使用 1 次 roll/hint（二选一）",
                            conf);
        }
        return true;
    };

    auto handle_guess_cd_set = [&]() {
        bool can_set = conf.p->is_op(conf.user_id) ||
                       (conf.message_type == "group" &&
                        is_group_op(conf.p, conf.group_id, conf.user_id));
        if (!can_set) {
            conf.p->cq_send("只有群管理员或OP可以设置冷却", conf);
            return true;
        }

        std::string arg =
            trim(message.substr(std::string("*guess_cd ").size()));
        int cd = static_cast<int>(my_string2int64(arg));
        if (cd < 0) {
            conf.p->cq_send("冷却次数不能小于0", conf);
            return true;
        }
        assist_cooldown_guess_by_scope_[id] = cd;
        save_cooldown_config();
        conf.p->cq_send("已设置本会话提示冷却为: " + std::to_string(cd), conf);
        return true;
    };

    auto handle_guess_help = [&]() {
        conf.p->cq_send(guessmap_detail_help(), conf);
        return true;
    };

    auto handle_guess_start = [&]() {
        load_maps();
        auto it = sessions_.find(id);
        if (it != sessions_.end() && it->second.active) {
            conf.p->cq_send("请先输入*guess_giveup结束目前的题目！", conf);
            return true;
        }

        const int crop = get_start_crop(message);
        if (!start_game(id, crop)) {
            conf.p->cq_send("题库未就绪，请检查 " + images_root_dir() + " 与 " +
                                maps_config_path(),
                            conf);
            return true;
        }
        send_all_hint_images(
            conf, sessions_[id],
            "这个截图是出自哪张图呢？\n输入*guess 你的答案 以回答");
        return true;
    };

    auto handle_guess_roll = [&]() {
        auto it = sessions_.find(id);
        if (it == sessions_.end() || !it->second.active) {
            return true;
        }

        const int cd = get_assist_cooldown_guess(id);
        if (!can_use_assist(it->second, cd)) {
            react_or_reply(conf, "424", "");
            conf.p->cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                                "]才刚提示过啦~再猜一猜！",
                            conf);
            return true;
        }

        if (!roll_hint(it->second, id, cd <= 0)) {
            conf.p->cq_send("roll次数已达上限或无法继续roll。", conf);
            return true;
        }
        if (cd > 0) {
            it->second.assist_used_count += 1;
        }
        send_all_hint_images(conf, it->second,
                             "已重roll同尺寸新位置，以下是当前全部提示图：");
        return true;
    };

    auto handle_guess_hint = [&]() {
        auto it = sessions_.find(id);
        if (it == sessions_.end() || !it->second.active) {
            return true;
        }

        const int cd = get_assist_cooldown_guess(id);
        if (!can_use_assist(it->second, cd)) {
            react_or_reply(conf, "424", "");
            conf.p->cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                                "]才刚提示过啦~再猜一猜！",
                            conf);
            return true;
        }

        if (!expand_hint(it->second, id, cd <= 0)) {
            conf.p->cq_send("hint次数已达上限或已到边界，无法继续扩大。", conf);
            return true;
        }
        if (cd > 0) {
            it->second.assist_used_count += 1;
        }
        send_all_hint_images(conf, it->second,
                             "已扩大提示范围，以下是当前全部提示图：");
        return true;
    };

    auto handle_guess_check = [&]() {
        load_maps();
        size_t img_count = 0;
        std::map<std::string, std::map<std::string, size_t>> grouped_counts;
        for (const auto &m : maps_) {
            img_count += m.images.size();
            const auto [collection, hall] =
                parse_collection_and_hall(m.file_path);
            grouped_counts[collection][hall] += 1;
        }

        std::ostringstream oss;
        oss << "guessmap状态: 可用地图=" << maps_.size()
            << ", 可用图片=" << img_count;
        if (!grouped_counts.empty()) {
            oss << "\n合集大厅统计(仅统计已存在截图的地图):";
            for (const auto &collection_it : grouped_counts) {
                size_t collection_total = 0;
                for (const auto &hall_it : collection_it.second) {
                    collection_total += hall_it.second;
                }
                oss << "\n- " << collection_it.first << " (" << collection_total
                    << ")";
                for (const auto &hall_it : collection_it.second) {
                    oss << "\n  - " << hall_it.first << ": " << hall_it.second;
                }
            }
        }
        conf.p->cq_send(oss.str(), conf);
        return true;
    };

    auto handle_guess_giveup = [&]() {
        auto it = sessions_.find(id);
        if (it == sessions_.end() || !it->second.active) {
            return true;
        }
        if (is_cooldown_active(id)) {
            conf.p->cq_send("刚开局，先猜一会再放弃吧。", conf);
            return true;
        }

        const session_state state = it->second;
        it->second.active = false;

        const std::string out = reveal_output_path(id);
        if (build_reveal_image(state, out)) {
            send_finish_message(conf,
                                "你放弃了！答案是：" +
                                    maps_[state.map_index].answer + "。",
                                out);
        }
        else {
            conf.p->cq_send("你放弃了！答案是：" +
                                maps_[state.map_index].answer + "。",
                            conf);
        }
        cleanup_generated_images(id);
        cooldown_.erase(id);
        sessions_.erase(id);
        return true;
    };

    auto handle_guess_answer = [&]() {
        auto it = sessions_.find(id);
        if (it == sessions_.end() || !it->second.active) {
            react_or_reply(conf, "10068", "?");
            return true;
        }

        const std::string arg = get_guess_arg(message);
        if (arg.empty()) {
            react_or_reply(conf, "10068", "?");
            return true;
        }

        session_state &live_state = it->second;
        const bool is_first_guess_of_user =
            live_state.guessed_users.insert(conf.user_id).second;
        live_state.total_guess_count += 1;

        const session_state state = live_state;
        const auto &entry = maps_[state.map_index];
        const std::wstring norm = normalize(arg);
        if (entry.aliases.find(norm) == entry.aliases.end()) {
            react_or_reply(conf, "424", "答案错误");
            if (state.total_guess_count % 8 == 0) {
                send_all_hint_images(conf, state,
                                     "你的猜测是错误的！你的题目是：");
            }
            return true;
        }

        it->second.active = false;
        const std::string out = reveal_output_path(id);
        const bool solved_fast = (std::chrono::steady_clock::now() -
                                  state.started_at) <= std::chrono::seconds(30);
        const bool should_alien_react = is_first_guess_of_user || solved_fast;
        if (should_alien_react) {
            react_or_reply(conf, "128125", "外星人啊");
        }
        else {
            react_or_reply(conf, "9989", "");
        }
        if (build_reveal_image(state, out)) {
            send_finish_message(
                conf, "你猜对了！答案是：" + entry.answer + "。", out);
        }
        else {
            conf.p->cq_send("你猜对了！答案是：" + entry.answer + "。", conf);
        }
        cleanup_generated_images(id);
        cooldown_.erase(id);
        sessions_.erase(id);
        return true;
    };

    const std::vector<cmd_exact_rule> exact_rules = {
        {"*guess_cd", handle_guess_cd},
        {"*guess_help", handle_guess_help},
        {"*guess.help", handle_guess_help},
        {"*guess_roll", handle_guess_roll},
        {"*guess_hint", handle_guess_hint},
        {"*guess_check", handle_guess_check},
        {"*guess_giveup", handle_guess_giveup},
    };

    const std::vector<cmd_prefix_rule> prefix_rules = {
        {"*guess_cd ", handle_guess_cd_set},
        {"*guess_start", handle_guess_start},
        {"*guess", handle_guess_answer},
    };

    if (cmd_dispatch(message, exact_rules, prefix_rules)) {
        return;
    }

    react_or_reply(conf, "10068", "?");
}

std::string guessmap::help()
{
    return "蔚蓝猜地图：根据截图猜地图。帮助：*guess.help";
}

DECLARE_FACTORY_FUNCTIONS(guessmap)
