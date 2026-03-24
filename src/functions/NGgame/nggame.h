#include "processable.h"
#include <map>
#include <vector>

enum gameState { idle, join, init, work };

typedef struct Player {
    userid_t id;
    userid_t pre_id;
    userid_t nex_id;
    std::string word;
    bool alive;
    Player(userid_t user_id)
    {
        id = user_id;
        pre_id = 0;
        nex_id = 0;
        word = "";
        alive = true;
    }
} player_t;

class NGGame {
private:
    gameState state;
    std::map<userid_t, player_t> ng;

public:
    NGGame() : state(gameState::idle) {}

    // player action
    bool join(userid_t user_id);
    bool set(userid_t user_id, std::string word);
    bool lose(userid_t user_id);
    bool quit(userid_t user_id);

    // game action
    void set_state(gameState next_state);
    void link();
    void abort();
    void send_list(const msg_meta &conf);
    void send_vic(const msg_meta &conf);

    // info
    gameState get_state();
    userid_t get_vic(userid_t user_id);
    userid_t get_winner();
    std::string get_ng(userid_t user_id);
    std::string get_info(userid_t user_id, const msg_meta &conf);
    std::string overall(const msg_meta &conf);
    std::vector<userid_t> get_lazy();
    std::vector<userid_t> get_players();

    bool is_alive(userid_t user_id);
    bool check_ng(std::string content, userid_t user_id);
    bool check_end();

    size_t ready_cnt();
    size_t alive_cnt();
    size_t total_cnt();
    size_t dead_cnt();
};

class NGgame : public processable {
public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
DECLARE_FACTORY_FUNCTIONS_HEADER