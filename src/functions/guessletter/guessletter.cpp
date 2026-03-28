#include "guessletter.h"

#include "utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
bool starts_with(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

std::string to_lower_ascii(std::string s)
{
    for (char &c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string spaced_mask(const std::string &s)
{
    std::ostringstream oss;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i) {
            oss << ' ';
        }
        oss << s[i];
    }
    return oss.str();
}

std::string display_name_in_group(const msg_meta &conf, userid_t uid)
{
    std::string name = get_username(conf.p, uid, conf.group_id);
    if (!trim(name).empty()) {
        return name;
    }
    return "用户" + std::to_string(uid);
}

void react_or_reply(const msg_meta &conf, const std::string &emoji_id,
                    const std::string &fallback_text)
{
    bool ok = false;
    try {
        Json::Value j;
        j["message_id"] = conf.message_id;
        j["emoji_id"] = emoji_id;
        Json::Value r =
            string_to_json(conf.p->cq_send("set_msg_emoji_like", j));
        ok = r["status"].asString() == "ok";
    }
    catch (...) {
    }

    if (!ok && !fallback_text.empty()) {
        conf.p->cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                            "]" + fallback_text,
                        conf);
    }
}

std::string guessletter_detail_help()
{
    return "蔚蓝开字母\n"
           "*kai.help: 查看本帮助\n"
           "*kai create: 创建房间\n"
           "*kai go: 直接开始自由模式（无需加入）\n"
           "*kai join: 加入房间\n"
           "*kai quit: 退出房间\n"
           "*kai count N: 设置题目数量\n"
           "*kai range pinyin|english|mixed|scope: 设置题目范围\n"
           "*kai scope <name>: 切换到指定合集（如 StrawberryJam）\n"
           "*kai scopes: 查看可用合集\n"
           "*kai start: 开始游戏\n"
           "*kai open <letter>: 开字母\n"
           "*kai guess <row> <answer>: 抢答某一题\n"
           "*kai score: 查看本局积分榜\n"
           "*kai status: 查看题板\n"
           "*kai abort: 终止游戏\n"
           "*kai reveal <row>: 揭露某一题";
}
} // namespace

guessletter::guessletter() { load_bank(); }

bool guessletter::is_ascii_alpha_num(char c)
{
    unsigned char u = static_cast<unsigned char>(c);
    return std::isalnum(u) != 0;
}

std::string guessletter::norm_key(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        out.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

std::string guessletter::norm_answer(const std::string &s)
{
    std::wstring w = string_to_wstring(s);
    std::wstring out;
    out.reserve(w.size());
    for (wchar_t c : w) {
        wchar_t lc = std::towlower(c);
        // Keep only useful core chars so guesses are tolerant to punctuation.
        if ((lc >= L'0' && lc <= L'9') || (lc >= L'a' && lc <= L'z') ||
            (lc >= 0x4E00 && lc <= 0x9FFF)) {
            out.push_back(lc);
        }
    }
    return wstring_to_string(out);
}

bool guessletter::load_bank()
{
    bank_.clear();
    Json::Value root = string_to_json(readfile(bank_path_, "{}"));
    Json::Value arr;
    if (root.isArray()) {
        arr = root;
    }
    else if (root.isObject() && root.isMember("entries") &&
             root["entries"].isArray()) {
        arr = root["entries"];
    }
    if (!arr.isArray()) {
        return false;
    }

    for (const auto &it : arr) {
        if (!it.isObject()) {
            continue;
        }
        bank_entry e;
        e.scope = trim(it.get("scope", "").asString());
        e.cn_name = trim(it.get("cn_name", "").asString());
        e.en_name = trim(it.get("en_name", "").asString());
        e.pinyin_key = norm_key(it.get("pinyin_key", "").asString());
        e.english_key = norm_key(it.get("english_key", "").asString());
        if (e.cn_name.empty() && e.en_name.empty()) {
            continue;
        }
        bank_.push_back(e);
    }

    return !bank_.empty();
}

bool guessletter::is_admin(const msg_meta &conf) const
{
    if (conf.p->is_op(conf.user_id)) {
        return true;
    }
    if (conf.message_type == "group") {
        return is_group_op(conf.p, conf.group_id, conf.user_id);
    }
    return false;
}

guessletter::session &guessletter::get_or_create_session(groupid_t gid,
                                                         userid_t host)
{
    auto it = sessions_.find(gid);
    if (it == sessions_.end()) {
        session s;
        s.host = host;
        s.players.push_back(host);
        sessions_[gid] = s;
    }
    return sessions_[gid];
}

bool guessletter::in_session(const session &s, userid_t uid) const
{
    return std::find(s.players.begin(), s.players.end(), uid) !=
           s.players.end();
}

std::string guessletter::render_question_line(int idx, const question &q,
                                              const msg_meta &conf) const
{
    std::ostringstream oss;
    oss << idx + 1 << ". " << spaced_mask(q.shown);
    if (q.solved) {
        std::string solver =
            q.solved_by ? display_name_in_group(conf, q.solved_by) : "系统";
        std::string cn = trim(q.cn_answer.empty() ? q.answer : q.cn_answer);
        oss << "\n[" << solver << "]: " << cn;
    }
    return oss.str();
}

std::string guessletter::render_board(const session &s,
                                      const msg_meta &conf) const
{
    std::ostringstream oss;
    oss << "题板(" << s.questions.size() << ")\n";
    for (size_t i = 0; i < s.questions.size(); ++i) {
        oss << render_question_line((int)i, s.questions[i], conf) << "\n";
    }
    if (!s.players.empty() && !s.started) {
        oss << "玩家: ";
        for (size_t i = 0; i < s.players.size(); ++i) {
            if (i) {
                oss << ",";
            }
            oss << display_name_in_group(conf, s.players[i]);
        }
        oss << "\n";
    }
    if (s.started && !s.free_mode && !s.players.empty()) {
        userid_t cur = s.players[s.round_idx % s.players.size()];
        oss << "当前轮到: [CQ:at,qq=" << cur << "]（"
            << display_name_in_group(conf, cur) << "）\n";
        oss << render_opened_letters(s);
    }
    else if (s.started && s.free_mode) {
        oss << "自由模式进行中\n";
        oss << render_opened_letters(s);
    }
    return trim(oss.str());
}

std::string guessletter::render_opened_letters(const session &s) const
{
    std::vector<char> letters(s.opened_letters.begin(), s.opened_letters.end());

    std::ostringstream oss;
    oss << "当前已开字母: ";
    if (letters.empty()) {
        oss << "（无）";
    }
    else {
        for (size_t i = 0; i < letters.size(); ++i) {
            if (i) {
                oss << ",";
            }
            oss << letters[i];
        }
    }
    return oss.str();
}

std::string guessletter::render_scoreboard(const session &s,
                                           const msg_meta &conf) const
{
    std::vector<std::pair<userid_t, int>> rows;
    rows.reserve(s.scores.size());
    for (const auto &kv : s.scores) {
        rows.push_back(kv);
    }
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first < b.first;
    });

    std::ostringstream oss;
    oss << "积分榜";
    if (rows.empty()) {
        oss << "：暂无积分";
        return oss.str();
    }
    oss << "：\n";
    for (size_t i = 0; i < rows.size(); ++i) {
        oss << i + 1 << ". " << display_name_in_group(conf, rows[i].first)
            << " - " << rows[i].second << "\n";
    }
    return trim(oss.str());
}

std::vector<guessletter::question>
guessletter::build_questions(const session &s) const
{
    std::vector<question> pool;
    std::string mode = s.range;
    std::string mode_norm = norm_key(mode);

    for (const auto &e : bank_) {
        if (mode != "mixed" && mode != "pinyin" && mode != "english") {
            if (norm_key(e.scope) != mode_norm) {
                continue;
            }
        }

        auto make_shown = [&](const std::string &key) {
            std::string shown;
            shown.reserve(key.size());
            for (char c : key) {
                shown.push_back(is_ascii_alpha_num(c) ? '_' : c);
            }
            return shown;
        };

        if ((mode == "mixed" || mode == "pinyin" ||
             (mode != "english" && mode != "pinyin" && mode != "mixed")) &&
            !e.cn_name.empty() && !e.pinyin_key.empty()) {
            question q;
            q.answer = e.cn_name;
            q.cn_answer = e.cn_name;
            q.key = e.pinyin_key;
            q.shown = make_shown(q.key);
            pool.push_back(q);
        }

        if ((mode == "mixed" || mode == "english" ||
             (mode != "english" && mode != "pinyin" && mode != "mixed")) &&
            !e.en_name.empty() && !e.english_key.empty()) {
            question q;
            q.answer = e.en_name;
            q.cn_answer = e.cn_name.empty() ? e.en_name : e.cn_name;
            q.key = e.english_key;
            q.shown = make_shown(q.key);
            pool.push_back(q);
        }
    }

    if (pool.empty()) {
        return {};
    }

    std::vector<question> out;
    std::vector<int> ids(pool.size());
    for (size_t i = 0; i < pool.size(); ++i) {
        ids[i] = (int)i;
    }

    const int n = std::min((int)pool.size(), std::max(1, s.question_count));
    for (int i = 0; i < n; ++i) {
        const int pos = get_random((int)ids.size());
        out.push_back(pool[ids[pos]]);
        ids.erase(ids.begin() + pos);
    }
    return out;
}

void guessletter::advance_turn(session &s)
{
    if (!s.players.empty()) {
        s.round_idx = (s.round_idx + 1) % (int)s.players.size();
    }
}

bool guessletter::all_solved(const session &s) const
{
    for (const auto &q : s.questions) {
        if (!q.solved) {
            return false;
        }
    }
    return true;
}

bool guessletter::check(std::string message, const msg_meta &conf)
{
    (void)conf;
    message = trim(message);
    return message.rfind("*kai", 0) == 0 || message.rfind("*ba", 0) == 0 ||
           message.rfind("*开字母", 0) == 0;
}

std::string guessletter::help()
{
    return "蔚蓝开字母：轮流开字母并抢答地图名。帮助：*kai.help";
}

void guessletter::process(std::string message, const msg_meta &conf)
{
    if (conf.message_type != "group") {
        return;
    }

    std::string m = trim(message);
    std::string cmd;
    if (starts_with(m, "*kai")) {
        cmd = trim(m.substr(4));
    }
    else if (starts_with(m, "*ba")) {
        cmd = trim(m.substr(3));
    }
    else if (starts_with(m, "*开字母")) {
        cmd = trim(m.substr(std::string("*开字母").size()));
    }
    else {
        return;
    }

    std::string lower_cmd = to_lower_ascii(cmd);

    std::lock_guard<std::mutex> guard(lock_);
    session &s = get_or_create_session(conf.group_id, conf.user_id);

    if (cmd.empty() || lower_cmd == "help" || lower_cmd == ".help" ||
        cmd == "帮助") {
        conf.p->cq_send(guessletter_detail_help(), conf);
        return;
    }

    if (lower_cmd == "create" || cmd == "组局") {
        if (s.started) {
            conf.p->cq_send("已有进行中的对局，请先结束当前对局。", conf);
            return;
        }
        sessions_.erase(conf.group_id);
        session &ns = get_or_create_session(conf.group_id, conf.user_id);
        conf.p->cq_send("已创建房间，发送 *kai join 加入。", conf);
        conf.p->cq_send(render_board(ns, conf), conf);
        return;
    }

    if (lower_cmd == "go") {
        if (s.started) {
            conf.p->cq_send("已有进行中的对局，请先结束当前对局。", conf);
            return;
        }
        s.host = conf.user_id;
        s.started = true;
        s.free_mode = true;
        s.round_idx = 0;
        s.players.clear();
        s.questions = build_questions(s);
        if (s.questions.empty()) {
            s.started = false;
            conf.p->cq_send("题库为空，请先离线生成: python3 "
                            "src/functions/guessletter/gen_guessletter_bank.py",
                            conf);
            return;
        }
        s.scores.clear();
        s.opened_letters.clear();
        s.last_open_at.clear();
        conf.p->cq_send("自由模式开始：无需加入，任何成员都可开字母或猜题。",
                        conf);
        conf.p->cq_send(render_board(s, conf), conf);
        return;
    }

    if (lower_cmd == "join" || cmd == "加入") {
        if (s.started) {
            conf.p->cq_send("游戏已开始，不能加入。", conf);
            return;
        }
        if (!in_session(s, conf.user_id)) {
            s.players.push_back(conf.user_id);
        }
        react_or_reply(conf, "9989", "加入成功。");
        return;
    }

    if (lower_cmd == "quit" || cmd == "退出") {
        if (!in_session(s, conf.user_id)) {
            return;
        }
        s.players.erase(
            std::remove(s.players.begin(), s.players.end(), conf.user_id),
            s.players.end());
        if (s.players.empty()) {
            sessions_.erase(conf.group_id);
            conf.p->cq_send("所有玩家已退出，房间已关闭。", conf);
            return;
        }
        s.round_idx %= (int)s.players.size();
        conf.p->cq_send("已退出当前房间。", conf);
        return;
    }

    if (starts_with(lower_cmd, "count ") || starts_with(cmd, "数量 ")) {
        if (s.started) {
            conf.p->cq_send("游戏已开始，不能修改题目数量。", conf);
            return;
        }
        std::string ns = starts_with(lower_cmd, "count ")
                             ? trim(cmd.substr(std::string("count ").size()))
                             : trim(cmd.substr(std::string("数量 ").size()));
        int n = (int)my_string2int64(ns);
        n = std::max(1, std::min(30, n));
        s.question_count = n;
        conf.p->cq_send(fmt::format("题目数量已设置为 {}", n), conf);
        return;
    }

    if (starts_with(lower_cmd, "range ") || starts_with(cmd, "范围 ")) {
        if (s.started) {
            conf.p->cq_send("游戏已开始，不能修改范围。", conf);
            return;
        }
        std::string r = starts_with(lower_cmd, "range ")
                            ? trim(cmd.substr(std::string("range ").size()))
                            : trim(cmd.substr(std::string("范围 ").size()));
        if (r.empty()) {
            return;
        }
        if (r == "拼音") {
            r = "pinyin";
        }
        else if (r == "英文") {
            r = "english";
        }
        else if (r == "混合") {
            r = "mixed";
        }
        s.range = r;
        conf.p->cq_send("范围已设置为: " + r, conf);
        return;
    }

    if (starts_with(lower_cmd, "scope ")) {
        if (s.started) {
            conf.p->cq_send("游戏已开始，不能修改范围。", conf);
            return;
        }
        std::string sc = trim(cmd.substr(std::string("scope ").size()));
        if (sc.empty()) {
            conf.p->cq_send("用法: *kai scope <name>", conf);
            return;
        }
        s.range = sc;
        conf.p->cq_send("已切换到合集范围: " + sc, conf);
        return;
    }

    if (lower_cmd == "scopes") {
        std::vector<std::string> scopes;
        for (const auto &e : bank_) {
            if (!trim(e.scope).empty()) {
                scopes.push_back(e.scope);
            }
        }
        std::sort(scopes.begin(), scopes.end());
        scopes.erase(std::unique(scopes.begin(), scopes.end()), scopes.end());
        if (scopes.empty()) {
            conf.p->cq_send("当前题库没有合集字段。", conf);
            return;
        }
        std::ostringstream oss;
        oss << "可用合集(" << scopes.size() << "): ";
        for (size_t i = 0; i < scopes.size(); ++i) {
            if (i) {
                oss << ", ";
            }
            oss << scopes[i];
        }
        conf.p->cq_send(oss.str(), conf);
        return;
    }

    if (lower_cmd == "score" || lower_cmd == "rank" || cmd == "积分榜") {
        conf.p->cq_send(render_scoreboard(s, conf), conf);
        return;
    }

    if (lower_cmd == "start" || cmd == "开始") {
        if (s.started) {
            conf.p->cq_send("游戏已经开始。", conf);
            return;
        }
        if (s.players.empty()) {
            s.players.push_back(conf.user_id);
        }
        s.questions = build_questions(s);
        if (s.questions.empty()) {
            conf.p->cq_send("题库为空，请先离线生成: python3 "
                            "src/functions/guessletter/gen_guessletter_bank.py",
                            conf);
            return;
        }
        s.scores.clear();
        s.opened_letters.clear();
        s.last_open_at.clear();
        s.started = true;
        s.free_mode = false;
        s.round_idx = 0;
        conf.p->cq_send("游戏开始：轮流开字母，任意玩家可抢答。", conf);
        conf.p->cq_send(render_board(s, conf), conf);
        return;
    }

    if (lower_cmd == "status" || cmd == "状态") {
        if (!s.started) {
            conf.p->cq_send("当前未开始。\n" + render_board(s, conf), conf);
        }
        else {
            conf.p->cq_send(render_board(s, conf), conf);
        }
        return;
    }

    if (lower_cmd == "abort" || cmd == "终止") {
        if (!is_admin(conf) && conf.user_id != s.host) {
            return;
        }
        sessions_.erase(conf.group_id);
        conf.p->cq_send("游戏已终止。", conf);
        return;
    }

    if (starts_with(lower_cmd, "reveal ") || starts_with(cmd, "揭露 ")) {
        if (!is_admin(conf) && conf.user_id != s.host) {
            return;
        }
        if (!s.started) {
            return;
        }
        std::string row_s =
            starts_with(lower_cmd, "reveal ")
                ? trim(cmd.substr(std::string("reveal ").size()))
                : trim(cmd.substr(std::string("揭露 ").size()));
        int row = (int)my_string2int64(row_s) - 1;
        if (row < 0 || row >= (int)s.questions.size()) {
            conf.p->cq_send("题号无效。", conf);
            return;
        }
        s.questions[row].solved = true;
        s.questions[row].shown = s.questions[row].key;
        s.questions[row].solved_by = 0;
        conf.p->cq_send(
            fmt::format("揭露第 {} 题：{}", row + 1, s.questions[row].answer),
            conf);
        if (all_solved(s)) {
            conf.p->cq_send(render_scoreboard(s, conf), conf);
            sessions_.erase(conf.group_id);
            conf.p->cq_send("所有题目已结束。", conf);
            return;
        }
        conf.p->cq_send(render_board(s, conf), conf);
        return;
    }

    if (starts_with(lower_cmd, "open ") || starts_with(cmd, "开 ")) {
        if (!s.started) {
            return;
        }
        if (!s.free_mode) {
            if (s.players.empty()) {
                return;
            }
            userid_t cur = s.players[s.round_idx % s.players.size()];
            if (conf.user_id != cur) {
                conf.p->cq_send("当前应由 [CQ:at,qq=" + std::to_string(cur) +
                                    "] 开字母。",
                                conf);
                return;
            }
        }
        else {
            const auto now = std::chrono::steady_clock::now();
            auto it = s.last_open_at.find(conf.user_id);
            if (it != s.last_open_at.end()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->second);
                if (elapsed.count() < 30) {
                    react_or_reply(conf, "424", "开字母太快了，请稍后再试。");
                    return;
                }
            }
            s.last_open_at[conf.user_id] = now;
        }

        std::string open_body =
            starts_with(lower_cmd, "open ")
                ? trim(cmd.substr(std::string("open ").size()))
                : trim(cmd.substr(2));
        std::string letter = trim(open_body);
        if (letter.empty()) {
            conf.p->cq_send("用法: *kai open <letter>", conf);
            return;
        }

        char ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(letter[0])));
        bool opened = false;
        for (auto &q : s.questions) {
            if (q.solved) {
                continue;
            }
            for (size_t i = 0; i < q.key.size(); ++i) {
                char kc = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(q.key[i])));
                if (kc == ch && q.shown[i] == '_') {
                    q.shown[i] = q.key[i];
                    opened = true;
                }
            }
        }
        if (is_ascii_alpha_num(ch)) {
            s.opened_letters.insert(ch);
        }
        if (!s.free_mode) {
            advance_turn(s);
        }
        const std::string tip =
            opened ? "已开出该字母（全题同步）" : "该字母不存在或已全部打开";
        conf.p->cq_send(tip + "\n" + render_board(s, conf), conf);
        return;
    }

    if (starts_with(lower_cmd, "guess ") || starts_with(cmd, "猜 ")) {
        if (!s.started) {
            return;
        }
        if (!s.free_mode && !in_session(s, conf.user_id)) {
            conf.p->cq_send("普通模式下需先加入游戏（*kai join）后才能猜题。",
                            conf);
            return;
        }
        std::string body = starts_with(lower_cmd, "guess ")
                               ? trim(cmd.substr(std::string("guess ").size()))
                               : trim(cmd.substr(2));
        std::istringstream iss(body);
        int row = 0;
        iss >> row;
        std::string answer;
        std::getline(iss, answer);
        answer = trim(answer);
        row -= 1;
        if (row < 0 || row >= (int)s.questions.size() || answer.empty()) {
            conf.p->cq_send("用法: *kai guess <row> <answer>", conf);
            return;
        }
        if (s.questions[row].solved) {
            conf.p->cq_send("该题已解。", conf);
            return;
        }

        if (norm_answer(answer) == norm_answer(s.questions[row].answer)) {
            s.questions[row].solved = true;
            s.questions[row].shown = s.questions[row].key;
            s.questions[row].solved_by = conf.user_id;
            s.scores[conf.user_id] += 1;
            conf.p->cq_send(fmt::format("回答正确！第 {} 题答案：{}", row + 1,
                                        s.questions[row].answer),
                            conf);
            if (all_solved(s)) {
                conf.p->cq_send(render_scoreboard(s, conf), conf);
                sessions_.erase(conf.group_id);
                conf.p->cq_send("全部题目已猜完，游戏结束。", conf);
                return;
            }
        }
        else {
            react_or_reply(conf, "424", "回答错误。");
        }
        conf.p->cq_send(render_board(s, conf), conf);
        return;
    }
}

DECLARE_FACTORY_FUNCTIONS(guessletter)
