#include "nggame.h"

#include <random>
#include <unordered_map>

static std::map<groupid_t, NGGame> games;
static std::unordered_map<userid_t, groupid_t> init_group_by_user;

static void clear_init_index_for_group(groupid_t group_id)
{
    for (auto it = init_group_by_user.begin();
         it != init_group_by_user.end();) {
        if (it->second == group_id) {
            it = init_group_by_user.erase(it);
        }
        else {
            ++it;
        }
    }
}

static void rebuild_init_index_for_group(groupid_t group_id, NGGame &game)
{
    clear_init_index_for_group(group_id);
    if (game.get_state() != gameState::init) {
        return;
    }
    for (userid_t uid : game.get_players()) {
        init_group_by_user[uid] = group_id;
    }
}

static bool starts_with(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

static userid_t extract_qq_from_at_segment(const std::string &seg)
{
    size_t qq_pos = seg.find("qq=");
    if (qq_pos == std::string::npos) {
        return 0;
    }
    qq_pos += 3;
    size_t qq_end = seg.find_first_of(",]", qq_pos);
    if (qq_end == std::string::npos || qq_end <= qq_pos) {
        return 0;
    }
    return my_string2uint64(seg.substr(qq_pos, qq_end - qq_pos));
}

static bool at_segment_matches_bot(const std::string &seg,
                                   const std::string &bot_qq)
{
    if (!starts_with(seg, "[CQ:at,") || seg.empty() || seg.back() != ']') {
        return false;
    }

    userid_t qq = extract_qq_from_at_segment(seg);
    return qq != 0 && qq == my_string2uint64(bot_qq);
}

static bool message_mentions_bot(const std::string &message,
                                 const std::string &bot_qq)
{
    size_t search_pos = 0;
    while (true) {
        size_t at_pos = message.find("[CQ:at,", search_pos);
        if (at_pos == std::string::npos) {
            break;
        }
        size_t at_end = message.find(']', at_pos);
        if (at_end == std::string::npos) {
            break;
        }
        std::string seg = message.substr(at_pos, at_end - at_pos + 1);
        if (at_segment_matches_bot(seg, bot_qq)) {
            return true;
        }
        search_pos = at_end + 1;
    }
    return false;
}

static std::string display_name_in_group(const msg_meta &conf, userid_t uid,
                                         groupid_t gid)
{
    std::string name = get_username(conf.p, uid, gid);
    if (!trim(name).empty()) {
        return name;
    }
    return "用户" + std::to_string(uid);
}

static void ng_react_or_reply(const msg_meta &conf, const std::string &emoji_id,
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

static std::string ng_detail_help()
{
    return "NG 游戏：\n"
           "*ng.help - 查看帮助\n"
           "*ng create - 创建房间\n"
           "*ng join - 加入房间\n"
           "*ng start - 分配目标并进入设置词阶段\n"
           "*ng go - 全员设置完成后开始游戏\n"
           "*ng guess <词> - 自检并出局\n"
           "*ng state | *ng quit [@用户] | *ng abort\n"
           "不要说挑战！想办法设局让群友不经意间说出量身设计的NG词吧~";
}

void send_msg_ng(bot *p, groupid_t group_id, userid_t user_id,
                 const std::string &content)
{
    msg_meta rep(group_id ? "group" : "private", user_id, group_id, 0, p);
    p->cq_send(content, rep);
}

static userid_t extract_first_at(const std::string &cq)
{
    size_t at_pos = cq.find("[CQ:at,");
    if (at_pos == std::string::npos) {
        return 0;
    }
    size_t at_end = cq.find(']', at_pos);
    if (at_end == std::string::npos) {
        return 0;
    }
    return extract_qq_from_at_segment(cq.substr(at_pos, at_end - at_pos + 1));
}

static std::string expand_at(std::string raw, const msg_meta &conf)
{
    std::string res = raw;
    size_t search_pos = 0;
    while (true) {
        size_t at_pos = res.find("[CQ:at,", search_pos);
        if (at_pos == std::string::npos) {
            break;
        }
        size_t at_end = res.find(']', at_pos);
        if (at_end == std::string::npos) {
            break;
        }

        std::string seg = res.substr(at_pos, at_end - at_pos + 1);
        userid_t qq = extract_qq_from_at_segment(seg);
        if (qq) {
            std::string replacement =
                "@" + get_username(conf.p, qq, conf.group_id);
            res.replace(at_pos, at_end - at_pos + 1, replacement);
            search_pos = at_pos + replacement.size();
        }
        else {
            search_pos = at_end + 1;
        }
    }
    return res;
}

bool NGGame::join(userid_t user_id)
{
    if (ng.find(user_id) != ng.end()) {
        return false;
    }
    ng.emplace(user_id, player_t(user_id));
    return true;
}

bool NGGame::set(userid_t user_id, std::string word)
{
    auto it = ng.find(user_id);
    if (it == ng.end()) {
        return false;
    }
    it->second.word = std::move(word);
    return true;
}

bool NGGame::lose(userid_t user_id)
{
    auto it = ng.find(user_id);
    if (it == ng.end() || !it->second.alive) {
        return false;
    }
    it->second.alive = false;
    return true;
}

bool NGGame::quit(userid_t user_id)
{
    auto it = ng.find(user_id);
    if (it == ng.end()) {
        return false;
    }

    userid_t pre_id = it->second.pre_id;
    userid_t nex_id = it->second.nex_id;

    if (pre_id != 0 && nex_id != 0 && pre_id != user_id && nex_id != user_id) {
        auto pre_it = ng.find(pre_id);
        auto nex_it = ng.find(nex_id);
        if (pre_it != ng.end() && nex_it != ng.end()) {
            pre_it->second.nex_id = nex_id;
            nex_it->second.pre_id = pre_id;
            nex_it->second.word = it->second.word;
        }
    }

    ng.erase(it);
    return true;
}

void NGGame::set_state(gameState next_state) { state = next_state; }

gameState NGGame::get_state() { return state; }

void NGGame::link()
{
    std::vector<userid_t> player_list;
    player_list.reserve(ng.size());
    for (const auto &it : ng) {
        player_list.push_back(it.first);
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(player_list.begin(), player_list.end(), rng);

    for (size_t i = 0; i < player_list.size(); i++) {
        userid_t cur = player_list[i];
        userid_t nex = player_list[(i + 1) % player_list.size()];
        ng.at(cur).nex_id = nex;
        ng.at(nex).pre_id = cur;
    }
}

void NGGame::abort()
{
    state = gameState::idle;
    ng.clear();
}

void NGGame::send_list(const msg_meta &conf)
{
    for (const auto &it : ng) {
        send_msg_ng(conf.p, 0, it.first, get_info(it.first, conf));
    }
}

void NGGame::send_vic(const msg_meta &conf)
{
    for (const auto &it : ng) {
        userid_t vic = get_vic(it.first);
        if (!vic) {
            continue;
        }
        send_msg_ng(conf.p, 0, it.first,
                    "请为 " + display_name_in_group(conf, vic, conf.group_id) +
                        " 设置 NG 词");
    }
}

std::string NGGame::get_ng(userid_t user_id)
{
    auto it = ng.find(user_id);
    if (it == ng.end()) {
        return "";
    }
    return it->second.word;
}

userid_t NGGame::get_vic(userid_t user_id)
{
    auto it = ng.find(user_id);
    if (it == ng.end()) {
        return 0;
    }
    if (it->second.nex_id == 0 || ng.find(it->second.nex_id) == ng.end()) {
        return 0;
    }
    return it->second.nex_id;
}

userid_t NGGame::get_winner()
{
    if (state != gameState::work) {
        return 0;
    }
    for (const auto &it : ng) {
        if (it.second.alive) {
            return it.first;
        }
    }
    return 0;
}

std::string NGGame::overall(const msg_meta &conf)
{
    std::string content;
    for (const auto &it : ng) {
        if (is_alive(it.first)) {
            content +=
                get_username(conf.p, it.first, conf.group_id) + ": alive\n";
        }
        else {
            content += get_username(conf.p, it.first, conf.group_id) + " -> " +
                       get_ng(it.first) + "\n";
        }
    }
    if (!content.empty()) {
        content.pop_back();
    }
    return content;
}

std::string NGGame::get_info(userid_t user_id, const msg_meta &conf)
{
    std::string content;
    for (const auto &it : ng) {
        if (it.first != user_id) {
            content += get_username(conf.p, it.first, conf.group_id) + " -> " +
                       it.second.word + "\n";
        }
    }
    if (!content.empty()) {
        content.pop_back();
    }
    return content;
}

std::vector<userid_t> NGGame::get_lazy()
{
    std::vector<userid_t> lazy;
    for (const auto &it : ng) {
        if (it.second.word.empty() && it.second.pre_id != 0) {
            lazy.push_back(it.second.pre_id);
        }
    }
    return lazy;
}

std::vector<userid_t> NGGame::get_players()
{
    std::vector<userid_t> players;
    players.reserve(ng.size());
    for (const auto &it : ng) {
        players.push_back(it.first);
    }
    return players;
}

size_t NGGame::ready_cnt()
{
    size_t cnt = 0;
    for (const auto &it : ng) {
        if (!it.second.word.empty()) {
            cnt++;
        }
    }
    return cnt;
}

bool NGGame::is_alive(userid_t user_id)
{
    auto it = ng.find(user_id);
    return it != ng.end() && it->second.alive;
}

bool NGGame::check_ng(std::string content, userid_t user_id)
{
    auto it = ng.find(user_id);
    if (state != gameState::work || it == ng.end() || !it->second.alive ||
        it->second.word.empty()) {
        return false;
    }
    return content.find(it->second.word) != std::string::npos;
}

bool NGGame::check_end()
{
    return state == gameState::work && alive_cnt() <= 1;
}

size_t NGGame::alive_cnt()
{
    size_t cnt = 0;
    for (const auto &it : ng) {
        if (it.second.alive) {
            cnt++;
        }
    }
    return cnt;
}

size_t NGGame::total_cnt() { return ng.size(); }

size_t NGGame::dead_cnt() { return total_cnt() - alive_cnt(); }

bool NGgame::check(std::string message, const msg_meta &conf)
{
    if (conf.message_type == "group") {
        std::string normalized = trim(message);
        const std::string bot_qq = std::to_string(conf.p->get_botqq());
        if (normalized == "*ng" || starts_with(normalized, "*ng ") ||
            message_mentions_bot(normalized, bot_qq)) {
            return true;
        }

        auto it = games.find(conf.group_id);
        return it != games.end() && it->second.get_state() == gameState::work;
    }

    if (conf.message_type == "private") {
        auto idx_it = init_group_by_user.find(conf.user_id);
        if (idx_it == init_group_by_user.end()) {
            return false;
        }

        auto game_it = games.find(idx_it->second);
        if (game_it == games.end() ||
            game_it->second.get_state() != gameState::init ||
            !game_it->second.get_vic(conf.user_id)) {
            init_group_by_user.erase(idx_it);
            return false;
        }
        return true;
    }

    return false;
}

void NGgame::process(std::string message, const msg_meta &conf)
{
    auto uid = conf.user_id;
    auto gid = conf.group_id;
    std::string bot_qq = std::to_string(conf.p->get_botqq());

    if (conf.message_type == "group") {
        auto game_it = games.find(gid);
        NGGame *game = (game_it == games.end() ? nullptr : &game_it->second);
        std::string cmd;
        bool is_ng_command = false;

        {
            std::string normalized = trim(message);

            // Remove bot-at CQ segments regardless of parameter order.
            size_t search_pos = 0;
            while (true) {
                size_t at_pos = normalized.find("[CQ:at,", search_pos);
                if (at_pos == std::string::npos) {
                    break;
                }
                size_t at_end = normalized.find(']', at_pos);
                if (at_end == std::string::npos) {
                    break;
                }
                std::string seg =
                    normalized.substr(at_pos, at_end - at_pos + 1);
                if (at_segment_matches_bot(seg, bot_qq)) {
                    normalized.erase(at_pos, at_end - at_pos + 1);
                    normalized = trim(normalized);
                    search_pos = 0;
                    continue;
                }
                search_pos = at_end + 1;
            }

            if (normalized == "*ng") {
                is_ng_command = true;
                cmd.clear();
            }
            else if (starts_with(normalized, "*ng ")) {
                is_ng_command = true;
                cmd = trim(normalized.substr(4));
            }
            else if (starts_with(normalized, "*ng.")) {
                is_ng_command = true;
                cmd = trim(normalized.substr(3));
            }
        }

        if (is_ng_command && starts_with(cmd, "guess")) {
            if (!game) {
                send_msg_ng(conf.p, gid, 0,
                            "当前没有房间，请先使用 *ng create。");
                return;
            }
            std::string guess;
            if (cmd.size() > 5) {
                guess = trim(cmd.substr(5));
            }
            if (game->check_ng(guess, uid)) {
                ng_react_or_reply(conf, "9989", "自检成功");
                send_msg_ng(conf.p, gid, 0, "恭喜！你的 NG 词是 " + guess);
                send_msg_ng(conf.p, gid, 0,
                            "游戏结束！获胜者是 " +
                                display_name_in_group(conf, uid, gid) + "！");
                clear_init_index_for_group(gid);
                game->abort();
            }
            else {
                ng_react_or_reply(conf, "12951", "自检失败");
                send_msg_ng(conf.p, gid, 0,
                            "很遗憾，你的 NG 词是 " + game->get_ng(uid) +
                                "\n下次加油！");
                game->lose(uid);
                if (game->check_end()) {
                    send_msg_ng(conf.p, gid, 0,
                                "游戏结束！获胜者是 " +
                                    display_name_in_group(
                                        conf, game->get_winner(), gid) +
                                    "！");
                    clear_init_index_for_group(gid);
                    game->abort();
                }
            }
            return;
        }

        if (game && game->get_state() == gameState::work) {
            std::string expanded_msg = expand_at(message, conf);
            if (game->check_ng(expanded_msg, uid)) {
                ng_react_or_reply(conf, "128165", "触发 NG 词");
                send_msg_ng(conf.p, gid, 0,
                            display_name_in_group(conf, uid, gid) +
                                "  Out！NG 词是 " + game->get_ng(uid));
                game->lose(uid);
                if (game->check_end()) {
                    send_msg_ng(conf.p, gid, 0,
                                "游戏结束！获胜者是 " +
                                    display_name_in_group(
                                        conf, game->get_winner(), gid) +
                                    "！");
                    clear_init_index_for_group(gid);
                    game->abort();
                }
            }
        }

        if (!is_ng_command) {
            return;
        }

        if (cmd == "create") {
            if (!game) {
                game_it = games.emplace(gid, NGGame()).first;
                game = &game_it->second;
            }
            if (game->get_state() != gameState::idle) {
                send_msg_ng(conf.p, gid, 0,
                            "已有游戏正在进行，可用 *ng state 查看状态。");
                return;
            }
            clear_init_index_for_group(gid);
            game->set_state(gameState::join);
            send_msg_ng(
                conf.p, gid, 0,
                "房间已创建。使用 *ng join 加入，人齐后 *ng start 开始选词。");
            return;
        }

        if (cmd == "start") {
            if (!game) {
                send_msg_ng(conf.p, gid, 0,
                            "当前没有房间，请先使用 *ng create。");
                return;
            }
            if (game->get_state() != gameState::join) {
                send_msg_ng(conf.p, gid, 0,
                            "*ng start 只能在创建并加入阶段使用。");
                return;
            }
            if (game->total_cnt() <= 1) {
                send_msg_ng(conf.p, gid, 0, "拉上朋友一起玩吧！");
                return;
            }
            game->set_state(gameState::init);
            game->link();
            rebuild_init_index_for_group(gid, *game);
            send_msg_ng(conf.p, gid, 0, "目标已分配，请私聊机器人设置 NG 词。");
            game->send_vic(conf);
            return;
        }

        if (cmd == "go") {
            if (!game) {
                send_msg_ng(conf.p, gid, 0,
                            "当前没有房间，请先使用 *ng create。");
                return;
            }
            if (game->get_state() != gameState::init) {
                send_msg_ng(conf.p, gid, 0,
                            "*ng go 只能在设置好 NG 词后使用。");
                return;
            }
            if (game->ready_cnt() != game->total_cnt()) {
                auto lazy_list = game->get_lazy();
                std::string content;
                for (auto it : lazy_list) {
                    content += display_name_in_group(conf, it, gid) + "、";
                }
                if (!content.empty()) {
                    content.pop_back();
                    content += " 还没有设置 NG 词";
                }
                else {
                    content = "仍有玩家未设置 NG 词。";
                }
                send_msg_ng(conf.p, gid, 0, content);
                return;
            }
            clear_init_index_for_group(gid);
            game->set_state(gameState::work);
            send_msg_ng(conf.p, gid, 0, "游戏开始！请查看私聊~");
            game->send_list(conf);
            return;
        }

        if (cmd == "join") {
            if (!game || game->get_state() == gameState::idle) {
                send_msg_ng(conf.p, gid, 0,
                            "当前没有房间，请先使用 *ng create。");
                return;
            }
            if (game->get_state() != gameState::join) {
                send_msg_ng(conf.p, gid, 0,
                            "当前阶段无法加入，请等待本局结束。");
                return;
            }
            if (!game->join(uid)) {
                send_msg_ng(conf.p, gid, 0, "你已经在本局游戏中了。");
                return;
            }
            send_msg_ng(conf.p, gid, 0,
                        display_name_in_group(conf, uid, gid) +
                            " 已加入，当前房间：" +
                            std::to_string(game->total_cnt()) +
                            "人 \n请添加Bot好友以便私聊设置 NG 词");
            return;
        }

        if (starts_with(cmd, "quit")) {
            if (!game) {
                send_msg_ng(conf.p, gid, 0,
                            "当前没有房间，请先使用 *ng create。");
                return;
            }
            userid_t victim =
                extract_first_at(message.substr(message.find("quit")));
            victim = victim ? victim : uid;
            if (!game->quit(victim)) {
                send_msg_ng(conf.p, gid, 0, "目标不在当前游戏中。");
                return;
            }
            send_msg_ng(conf.p, gid, 0,
                        display_name_in_group(conf, victim, gid) +
                            " 已退出游戏。");
            if (game->get_state() == gameState::init) {
                rebuild_init_index_for_group(gid, *game);
            }
            if (game->check_end()) {
                send_msg_ng(
                    conf.p, gid, 0,
                    "游戏结束！获胜者是 " +
                        display_name_in_group(conf, game->get_winner(), gid) +
                        "！");
                clear_init_index_for_group(gid);
                game->abort();
            }
            return;
        }

        if (cmd == "abort") {
            if (!game) {
                send_msg_ng(conf.p, gid, 0, "当前没有进行中的游戏。");
                return;
            }
            clear_init_index_for_group(gid);
            game->abort();
            send_msg_ng(conf.p, gid, 0, "游戏已终止。");
            return;
        }

        if (cmd == "state") {
            if (!game) {
                send_msg_ng(conf.p, gid, 0, "当前没有进行中的游戏。");
                return;
            }
            switch (game->get_state()) {
            case gameState::idle:
                send_msg_ng(conf.p, gid, 0, "当前没有进行中的游戏。");
                break;
            case gameState::join:
                send_msg_ng(conf.p, gid, 0,
                            "加入阶段：" + std::to_string(game->total_cnt()) +
                                " 人。");
                break;
            case gameState::init:
                send_msg_ng(conf.p, gid, 0,
                            "设置 NG 词阶段（" +
                                std::to_string(game->ready_cnt()) + "/" +
                                std::to_string(game->total_cnt()) + "）");
                send_msg_ng(conf.p, 0, uid,
                            "你的目标是 " + display_name_in_group(
                                                conf, game->get_vic(uid), gid));
                break;
            case gameState::work:
                send_msg_ng(conf.p, gid, 0, game->overall(conf));
                send_msg_ng(conf.p, 0, uid, game->get_info(uid, conf));
                break;
            default:
                break;
            }
            return;
        }

        if (cmd == "help" || cmd == ".help") {
            send_msg_ng(conf.p, gid, 0, ng_detail_help());
            return;
        }

        send_msg_ng(conf.p, gid, 0, "未知命令，请使用 *ng.help 查看帮助。");
        return;
    }

    if (conf.message_type == "private") {
        auto idx_it = init_group_by_user.find(uid);
        if (idx_it == init_group_by_user.end()) {
            return;
        }

        auto game_it = games.find(idx_it->second);
        if (game_it == games.end() ||
            game_it->second.get_state() != gameState::init) {
            init_group_by_user.erase(idx_it);
            return;
        }

        groupid_t group_id = game_it->first;
        auto &game = game_it->second;
        userid_t vic = game.get_vic(uid);
        if (!vic) {
            init_group_by_user.erase(idx_it);
            return;
        }

        game.set(vic, message);
        size_t ready = game.ready_cnt();
        size_t total = game.total_cnt();

        std::string victim_name = display_name_in_group(conf, vic, group_id);
        std::string setter_name = display_name_in_group(conf, uid, group_id);

        send_msg_ng(conf.p, 0, uid,
                    "已为 " + victim_name + " 设置 NG 词：" + message);
        send_msg_ng(conf.p, group_id, 0,
                    setter_name + " 已设置 NG 词！（" + std::to_string(ready) +
                        "/" + std::to_string(total) + ")");
        return;
    }
}

std::string NGgame::help()
{
    return "NG 游戏：不要说挑战。帮助：*ng.help";
}

DECLARE_FACTORY_FUNCTIONS(NGgame)
