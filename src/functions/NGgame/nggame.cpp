#include "nggame.h"

static std::map<int64_t, NGame> games;

void NGgame::process(std::string message, const msg_meta &conf)
{
    if (conf.message_type == "group")
    {
        if (games[conf.group_id].get_state() == gameState::work && games[conf.group_id].is_alive(conf.user_id))
        {
            std::string ngword = games[conf.group_id].get_ng(conf.user_id);
            if (message.find(ngword) != message.npos)
            {
                std::string content =
                    get_username(conf.p, conf.user_id, conf.group_id) + " Out! The NG word is " + ngword;
                msg_meta rep;
                rep.p = conf.p;
                rep.group_id = conf.group_id;
                rep.message_type = "group";
                conf.p->cq_send(content, rep);
                if (games[conf.group_id].get_alive_num() <= 1)
                {
                    content = "Game Over! The winner is" + content;
                    conf.p->cq_send(content, rep);
                    games[conf.group_id].clear();
                    games[conf.group_id].update(true);
                }
            }
        }
        if (message.find("[CQ:at,qq=" + std::to_string(conf.p->get_botqq()) + "] ng start") != message.npos)
        {
            switch (games[conf.group_id].get_state())
            {
            case gameState::idle: {
                games[conf.group_id].clear();
                games[conf.group_id].update(false);
                msg_meta rep;
                rep.p = conf.p;
                rep.group_id = conf.group_id;
                rep.message_type = "group";
                conf.p->cq_send("Send \"ng join\" to join", rep);
                break;
            }
            case gameState::join: {
                msg_meta rep;
                rep.p = conf.p;
                rep.group_id = conf.group_id;
                rep.message_type = "group";
                conf.p->cq_send("Send the NG words via private chat.", rep);
                // collect ng words
                std::vector<int64_t> v = games[conf.group_id].get_player_list();
                std::random_shuffle(v.begin(), v.end());
                for (size_t i = 0; i < v.size() - 1; i++)
                {
                    games[conf.group_id].link[v[i]] = v[i + 1];
                }
                games[conf.group_id].link[v[v.size() - 1]] = v[0];
                for (auto it : games[conf.group_id].link)
                {
                    msg_meta f;
                    f.message_type = "private";
                    f.user_id = it.first;
                    f.p = conf.p;
                    std::string content = "Please set NG word for " + get_username(conf.p, it.second, conf.group_id);
                    conf.p->cq_send(content, f);
                }
                games[conf.group_id].update(false);
                break;
            }
            case gameState::init: {
                if (games[conf.group_id].is_ready())
                {
                    msg_meta rep;
                    rep.p = conf.p;
                    rep.group_id = conf.group_id;
                    rep.message_type = "group";
                    conf.p->cq_send("NG game starts!", rep);
                    games[conf.group_id].update(false);
                }
                else
                {
                    msg_meta rep;
                    rep.p = conf.p;
                    rep.group_id = conf.group_id;
                    rep.message_type = "group";
                    conf.p->cq_send("Someone has no NG word yet!", rep);
                }
                break;
            }
            case gameState::work: {
                msg_meta rep;
                rep.p = conf.p;
                rep.group_id = conf.group_id;
                rep.message_type = "group";
                conf.p->cq_send("NG game already started!", rep);
                break;
            }
            default: {
                msg_meta rep;
                rep.p = conf.p;
                rep.group_id = conf.group_id;
                rep.message_type = "group";
                conf.p->cq_send("This should not happen.", rep);
            }
            }
        }
        else if (message.find("[CQ:at,qq=" + std::to_string(conf.p->get_botqq()) + "] ng abort") != message.npos)
        {
            msg_meta rep;
            rep.p = conf.p;
            rep.group_id = conf.group_id;
            rep.message_type = "group";
            conf.p->cq_send("NG game aborts!", rep);
            games[conf.group_id].clear();
        }
        else if (message.find("[CQ:at,qq=" + std::to_string(conf.p->get_botqq()) + "] ng state") != message.npos)
        {
            std::string content = "";
            if (games[conf.group_id].get_state() != gameState::work)
            {
                for (auto it2 : games[conf.group_id].ng)
                {
                    if (it2.first != conf.user_id)
                    {
                        content += get_username(conf.p, it2.first, conf.group_id) + " -> " + it2.second + "\n";
                    }
                }
                msg_meta rep;
                rep.p = conf.p;
                rep.user_id = conf.user_id;
                rep.message_type = "private";
                conf.p->cq_send(content, rep);
            }
            else
            {
                msg_meta rep;
                rep.p = conf.p;
                rep.group_id = conf.group_id;
                rep.message_type = "group";
                conf.p->cq_send("No Ongoing Game!", rep);
            }
        }
        else if (message.find("[CQ:at,qq=" + std::to_string(conf.p->get_botqq()) + "] ng join") != message.npos)
        {
            if (games[conf.group_id].get_state() == gameState::join)
            {
                if (!games[conf.group_id].add_member(conf.user_id))
                {
                    msg_meta rep;
                    rep.p = conf.p;
                    rep.group_id = conf.group_id;
                    rep.message_type = "group";
                    conf.p->cq_send("You are already in this game!", rep);
                }
            }
        }
    }
    else if (conf.message_type == "private")
    {
        for (auto it : games)
        {
            if (it.second.get_state() == gameState::init)
            {
                for (auto pit : it.second.ng)
                {
                    if (pit.first == conf.user_id) // gamer
                    {
                        int64_t victim = it.second.link[conf.user_id];
                        it.second.ng[victim] = message;
                        msg_meta rep;
                        rep.p = conf.p;
                        rep.user_id = conf.user_id;
                        rep.message_type = "private";
                        std::string content =
                            "Set NG word for " + get_username(conf.p, victim, it.first) + ": " + message;
                        conf.p->cq_send(content, rep);

                        size_t ng_set_cnt = 0;
                        for (auto sit : it.second.ng)
                        {
                            if (sit.second.compare("") != 0)
                            {
                                ng_set_cnt++;
                            }
                        }
                        rep.group_id = it.first;
                        rep.message_type = "group";
                        content = get_username(conf.p, conf.user_id, it.first) + "has set the NG word! (" +
                                  std::to_string(ng_set_cnt) + "/" + std::to_string(it.second.ng.size()) + ")";
                        if (ng_set_cnt == it.second.ng.size())
                        {
                            content = content + "\nSent @Bot ng start to start";
                        }
                        conf.p->cq_send(content, rep);
                    }
                }
            }
        }
    }
}

bool NGgame::check(std::string message, const msg_meta &conf)
{
    return true;
}

std::string NGgame::help()
{
    return "@Bot ng start\n@Bot ng join\n@Bot ng state\n@Bot ng abort";
}

void NGame::clear()
{
    state = gameState::idle;
    dead.clear();
    ng.clear();
    link.clear();
}

void NGame::set_ngword(int64_t user_id, std::string ngword)
{
    ng[user_id] = ngword;
}

bool NGame::add_member(int64_t user_id)
{
    auto it = ng.find(user_id);
    if (it == ng.end())
    {
        set_ngword(user_id, "");
        return true;
    }
    return false;
}

bool NGame::is_ready()
{
    for (auto it : ng)
    {
        if (it.second.compare("") == 0)
        {
            return false;
        }
    }
    return true;
}

void NGame::out(int64_t user_id)
{
    dead.push_back(user_id);
}

bool NGame::is_alive(int64_t user_id)
{
    for (auto it : dead)
    {
        if (it == user_id)
        {
            return false;
        }
    }
    return true;
}

std::string NGame::get_ng(int64_t user_id)
{
    return ng[user_id];
}

int NGame::get_alive_num()
{
    return ng.size() - dead.size();
}

int NGame::get_player_num()
{
    return ng.size();
}

gameState NGame::get_state()
{
    return state;
}

void NGame::update(bool toabort)
{
    if (toabort)
    {
        state = idle;
    }
    else
    {
        state = gameState(state + 1);
    }
}

std::vector<int64_t> NGame::get_player_list()
{
    std::vector<int64_t> l;
    for (auto it : ng)
    {
        l.push_back(it.first);
    }
    return l;
}