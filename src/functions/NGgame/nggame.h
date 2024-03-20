#include "processable.h"
#include <algorithm>
#include <map>
#include <vector>

enum gameState
{
    idle,
    join,
    init,
    work
};

class NGame
{
  private:
    gameState state;
    std::vector<uint64_t> dead;

  public:
    std::map<uint64_t, std::string> ng;
    std::map<uint64_t, uint64_t> link;
    void clear();
    void set_ngword(uint64_t user_id, std::string ngword);
    bool add_member(uint64_t user_id);
    void set_link();

    bool is_ready();
    void out(uint64_t user_id);
    bool is_alive(uint64_t user_id);
    std::string get_ng(uint64_t user_id);
    int get_alive_num();
    int get_player_num();

    std::vector<uint64_t> get_player_list();
    gameState get_state();
    void update(bool toabort);

    std::string get_info(uint64_t user_id);
};

class NGgame : public processable
{
  public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
