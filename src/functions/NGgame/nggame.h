#include "processable.h"
#include <algorithm>
#include <chrono>
#include <map>
#include <vector>

enum gameState
{
    idle,
    join,
    init,
    work
};

typedef struct Player player_t;

struct Player
{
    uint64_t id;
    player_t *pre;
    player_t *nex;
    std::string word;
    bool alive;
    Player(uint64_t user_id)
    {
        id = user_id;
        pre = nex = NULL;
        word = "";
        alive = true;
    }
};

class NGGame
{
  private:
    gameState state;
    std::map<uint64_t, player_t *> ng;

  public:
    // player action
    bool join(uint64_t user_id);
    bool set(uint64_t user_id, std::string word);
    bool lose(uint64_t user_id);
    bool quit(uint64_t user_id);

    // game action
    void set_state(gameState next_state);
    void link();
    void abort();
    void send_list(const msg_meta &conf);
    void send_vic(const msg_meta &conf);

    // info
    gameState get_state();
    uint64_t get_vic(uint64_t user_id);
    uint64_t get_winner();
    std::string get_ng(uint64_t user_id);
    std::string get_info(uint64_t user_id, const msg_meta &conf);
    std::string overall(const msg_meta &conf);
    std::vector<uint64_t> get_lazy();


    bool is_alive(uint64_t user_id);
    bool check_ng(std::string content, uint64_t user_id);
    bool check_end();

    size_t ready_cnt();
    size_t alive_cnt();
    size_t total_cnt();
    size_t dead_cnt();
};

class NGgame : public processable
{
  public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};

extern "C" processable* create();