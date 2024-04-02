#include "nggame.h"

static std::string help_url = "https://demo.hedgedoc.org/zSOS-JMKSCey2E_4UjrrOQ?view";

void send_msg_ng(bot *p, uint64_t group_id, uint64_t user_id, std::string content);

bool NGGame::join(uint64_t user_id)
{
    if (ng.find(user_id) != ng.end())
    {
        return false;
    }
    ng[user_id] = new player(user_id);
    return true;
}

bool NGGame::set(uint64_t user_id, std::string word)
{
    if (ng.find(user_id) == ng.end())
    {
        return false;
    }
    ng[user_id]->word = word;
    return true;
}

bool NGGame::lose(uint64_t user_id)
{
    if (ng.find(user_id) == ng.end() || !ng[user_id]->alive)
    {
        return false;
    }
    ng[user_id]->alive = false;
    return true;
}

bool NGGame::quit(uint64_t user_id)
{
    if (ng.find(user_id) == ng.end())
    {
        return false;
    }
    // after linking, nex inherite the word from pre
    if (ng[user_id]->nex)
    {
        ng[user_id]->pre->nex = ng[user_id]->nex;
        ng[user_id]->nex->pre = ng[user_id]->pre;
        ng[user_id]->nex->word = ng[user_id]->word;
    }
    delete ng[user_id];
    ng[user_id] = NULL;
    ng.erase(user_id);
    return true;
}

void NGGame::set_state(gameState next_state)
{
    state = next_state;
}

void NGGame::link()
{
    std::vector<uint64_t> player_list;
    for (auto it : ng)
    {
        player_list.push_back(it.first);
    }
    srand(time(0));
    std::random_shuffle(player_list.begin(), player_list.end());
    for (size_t i = 0; i < player_list.size(); i++)
    {
        ng[player_list[i]]->nex = ng[player_list[(i + 1) % player_list.size()]];
        ng[player_list[(i + 1) % player_list.size()]]->pre = ng[player_list[i]];
    }
    player_list.clear();
    player_list.shrink_to_fit();
}

void NGGame::abort()
{
    state = gameState::idle;
    for (auto it : ng)
    {
        delete it.second;
        it.second = NULL;
    }
    ng.clear();
}

void NGGame::send_list(const msg_meta &conf)
{
    for (auto it : ng)
    {
        send_msg_ng(conf.p, 0, it.first, get_info(it.first, conf));
    }
}

void NGGame::send_vic(const msg_meta &conf)
{
    for (auto it : ng)
    {
        send_msg_ng(conf.p, 0, it.first,
                    "Pls set NG word for " + get_username(conf.p, it.second->nex->id, conf.group_id));
    }
}

gameState NGGame::get_state()
{
    return state;
}

std::string NGGame::get_ng(uint64_t user_id)
{
    if (ng.find(user_id) == ng.end() || !ng[user_id])
    {
        return "";
    }
    return ng[user_id]->word;
}

uint64_t NGGame::get_vic(uint64_t user_id)
{
    if (ng.find(user_id) == ng.end() || !ng[user_id]->nex)
    {
        return 0;
    }
    return ng[user_id]->nex->id;
}

uint64_t NGGame::get_winner()
{
    if (state == gameState::work)
    {
        for (auto it : ng)
        {
            if (it.second->alive)
            {
                return it.first;
            }
        }
    }
    return 0;
}

std::string NGGame::overall(const msg_meta &conf)
{
    std::string content = "";
    for (auto it : ng)
    {
        if (is_alive(it.first))
        {
            content = content + get_username(conf.p, it.first, conf.group_id) + ": alive\n";
        }
        else
        {
            content = content + get_username(conf.p, it.first, conf.group_id) + " -> " + get_ng(it.first) + "\n";
        }
    }
    if (content.length())
    {
        content.pop_back();
    }
    return content;
}

std::string NGGame::get_info(uint64_t user_id, const msg_meta &conf)
{
    std::string content = "";
    for (auto it : ng)
    {
        if (it.first != user_id)
        {
            content = content + get_username(conf.p, it.first, conf.group_id) + " -> " + it.second->word + "\n";
        }
    }
    content.pop_back();
    return content;
}

std::vector<uint64_t> NGGame::get_lazy()
{
    std::vector<uint64_t> lazy;
    for (auto it : ng)
    {
        if (!it.second->word.compare(""))
        {
            lazy.push_back(it.second->pre->id);
        }
    }
    return lazy;
}

size_t NGGame::ready_cnt()
{
    size_t cnt = 0;
    for (auto it : ng)
    {
        if (it.second && it.second->word.compare(""))
        {
            cnt++;
        }
    }
    return cnt;
}

bool NGGame::is_alive(uint64_t user_id)
{
    if (ng.find(user_id) == ng.end() || !ng[user_id]->alive)
    {
        return false;
    }
    return true;
}

bool NGGame::check_ng(std::string content, uint64_t user_id)
{
    if (state != gameState::work || ng.find(user_id) == ng.end() || !ng[user_id]->alive ||
        content.find(ng[user_id]->word) == std::string::npos)
    {
        return false;
    }
    return true;
}

bool NGGame::check_end()
{
    return state == gameState::work && alive_cnt() <= 1;
}

size_t NGGame::alive_cnt()
{
    size_t cnt = 0;
    for (auto it : ng)
    {
        if (it.second && it.second->alive)
        {
            cnt++;
        }
    }
    return cnt;
}

size_t NGGame::total_cnt()
{
    return ng.size();
}

size_t NGGame::dead_cnt()
{
    return total_cnt() - alive_cnt();
}

static std::map<uint64_t, NGGame> games;
static msg_meta rep;

void send_msg_ng(bot *p, uint64_t group_id, uint64_t user_id, std::string content)
{
    rep.user_id = user_id;
    rep.group_id = group_id;
    rep.message_type = group_id ? "group" : "private";
    p->cq_send(content, rep);
}

static uint64_t extract_at(std::string cq)
{
    if (cq.find("[CQ:at,qq=") == std::string::npos || cq.find("]") == std::string::npos)
    {
        return 0;
    }
    size_t st = cq.find("=") + 1;
    size_t ed = cq.find("]");
    return my_string2uint64(cq.substr(st, ed - st));
}

static std::string expand_at(std::string raw, const msg_meta &conf)
{
    std::string res = raw;
    size_t pos1, pos2;
    while ((pos1 = res.find("[CQ:at,qq=")) != std::string::npos || (pos2=res.find("]")) != std::string::npos)
    {
        uint64_t qq = extract_at(res);
        if (qq)
        {
            res = res.substr(0, pos1) + "@" + get_username(conf.p, qq, conf.group_id) + res.substr(pos2 + 1);
        }
    }
    return res;
}

void NGgame::process(std::string message, const msg_meta &conf)
{
    auto uid = conf.user_id;
    auto gid = conf.group_id;
    std::string prefix = "[CQ:at,qq=" + std::to_string(conf.p->get_botqq()) + "] ng ";
    if (conf.message_type == "group")
    {
        auto &game = games[conf.group_id];
        if (message.find(prefix + "guess") != std::string::npos) // guess
        {
            std::string guess = message.substr(message.find("guess") + 6);
            if (game.check_ng(guess, uid))
            {
                send_msg_ng(conf.p, gid, 0, "Congrats! Your NG word is " + guess);
                send_msg_ng(conf.p, gid, 0, "Game over! The winner is " + get_username(conf.p, uid, gid));
                game.abort();
            }
            else
            {
                send_msg_ng(conf.p, gid, 0, "Oops, your NG word is " + game.get_ng(uid) + "\nGood luck next time!");
                game.lose(uid);
                if (game.check_end())
                {
                    send_msg_ng(conf.p, gid, 0,
                                "Game over! The winner is " + get_username(conf.p, game.get_winner(), gid));
                    game.abort();
                }
            }
        }
        else // normal check
        {
            std::string expanded_msg = expand_at(message, conf);
            if (game.check_ng(expanded_msg, uid))
            {
                send_msg_ng(conf.p, gid, 0,
                            get_username(conf.p, uid, gid) + " Out! The NG word is " + game.get_ng(uid));
                game.lose(uid);
                if (game.check_end())
                {
                    send_msg_ng(conf.p, gid, 0,
                                "Game over! The winner is " + get_username(conf.p, game.get_winner(), gid));
                    game.abort();
                }
            }
        }
        if (message.find(prefix + "start") != std::string::npos)
        {
            switch (game.get_state())
            {
            case gameState::idle: {
                game.set_state(gameState::join);
                send_msg_ng(conf.p, gid, 0, "Game created, send @Bot ng join to join\nHelp: " + help_url);
                break;
            }
            case gameState::join: {
                if (game.total_cnt() <= 1)
                {
                    send_msg_ng(conf.p, gid, 0, "Bocchi-the-NG-Game");
                }
                else
                {
                    game.set_state(gameState::init);
                    game.link();
                    send_msg_ng(conf.p, gid, 0, "Pls send the NG words via private chats");
                    game.send_vic(conf);
                }
                break;
            }
            case gameState::init: {
                if (game.ready_cnt() == game.total_cnt())
                {
                    game.set_state(gameState::work);
                    send_msg_ng(conf.p, gid, 0, "Game starts! Pls check your private chat for the NG words");
                    game.send_list(conf);
                }
                else
                {
                    auto lazy_list = game.get_lazy();
                    std::string content = "";
                    for (auto it : lazy_list)
                    {
                        content = content + get_username(conf.p, it, gid) + ",";
                    }
                    content.pop_back();
                    if (lazy_list.size() > 1)
                    {
                        content = content + " has not set the NG word yet";
                    }
                    else
                    {
                        content = content + " have not set the NG words yet";
                    }
                    send_msg_ng(conf.p, gid, 0, content);
                }
                break;
            }
            case gameState::work: {
                send_msg_ng(conf.p, gid, 0, "Game already started");
                break;
            }
            default:
                break;
            }
        }
        else if (message.find(prefix + "abort") != std::string::npos)
        {
            game.abort();
            send_msg_ng(conf.p, gid, 0, "Game aborted.");
        }
        else if (message.find(prefix + "state") != std::string::npos)
        {
            switch (game.get_state())
            {
            case gameState::idle: {
                send_msg_ng(conf.p, gid, 0, "No ongoing game");
                break;
            }
            case gameState::join: {
                send_msg_ng(conf.p, gid, 0,
                            "A game is collecting players, currently " + std::to_string(game.total_cnt()) + " player" +
                                (game.total_cnt() > 1 ? "s" : ""));
                break;
            }
            case gameState::init: {
                send_msg_ng(conf.p, gid, 0,
                            "A game is collecting NG words (" + std::to_string(game.ready_cnt()) + "/" +
                                std::to_string(game.total_cnt()) + ")");
                send_msg_ng(conf.p, 0, uid, "Your victim is " + get_username(conf.p, game.get_vic(uid), gid));
                break;
            }
            case gameState::work: {
                send_msg_ng(conf.p, gid, 0, game.overall(conf));
                send_msg_ng(conf.p, 0, uid, game.get_info(uid, conf));
                break;
            }
            default: {
                // should not execute here
            }
            }
        }
        else if (message.find(prefix + "join") != std::string::npos)
        {
            if(game.get_state() == gameState::idle)
            {
                send_msg_ng(conf.p, gid, 0, "No existed game. Send @Bot ng start to create one");
            }
            else if (game.get_state() != gameState::join)
            {
                send_msg_ng(conf.p, gid, 0, "Pls wait for the ongoing game to terminate");
            }
            else if (game.join(uid))
            {
                send_msg_ng(conf.p, gid, 0,
                            get_username(conf.p, uid, gid) + " joins the game!\nCurrently " +
                                std::to_string(game.total_cnt()) + " in this game\nPls add the bot as friend to play");
            }
            else
            {
                send_msg_ng(conf.p, gid, 0, "You are already in this game");
            }
        }
        else if ((message.find(prefix + "quit") != std::string::npos))
        {
            uint64_t victim = extract_at(message.substr(message.find("quit")));
            victim = victim ? victim : uid;
            if (game.quit(victim))
            {
                send_msg_ng(conf.p, gid, 0,
                            get_username(conf.p, victim, gid) +
                                " left the game\nSend @Bot ng state to check the updated NG words");
                if (game.check_end())
                {
                    send_msg_ng(conf.p, gid, 0,
                                "Game over! The winner is " + get_username(conf.p, game.get_winner(), gid));
                    game.abort();
                }
            }
            else
            {
                send_msg_ng(conf.p, gid, 0, "Currently you are not in this game");
            }
        }
        else if ((message.find(prefix + "help") != std::string::npos))
        {
            send_msg_ng(conf.p, gid, 0, "NG Game Help: " + help_url);
        }
    }
    else if (conf.message_type == "private")
    {
        for (auto g : games)
        {
            auto &game = g.second;
            if (game.get_state() == gameState::init)
            {
                uint64_t vic = game.get_vic(uid);
                if (vic)
                {
                    game.set(vic, message);
                    send_msg_ng(conf.p, 0, uid,
                                "Set NG word for" + get_username(conf.p, vic, g.first) + ": " + message);
                    send_msg_ng(conf.p, g.first, 0,
                                get_username(conf.p, uid, g.first) + " has set the NG word!(" +
                                    std::to_string(game.ready_cnt()) + "/" + std::to_string(game.total_cnt()) + ")");
                }
            }
        }
    }
    else
    {
        // should not happen
    }
}

bool NGgame::check(std::string message, const msg_meta &conf)
{
    return true;
}

std::string NGgame::help()
{
    return "NG Game Help: " + help_url;
}