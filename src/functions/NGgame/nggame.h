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
    std::vector<int64_t> dead;

  public:
    std::map<int64_t, std::string> ng;
    std::map<int64_t, int64_t> link;
    void clear();
    void set_ngword(int64_t user_id, std::string ngword);
    bool add_member(int64_t user_id);
    void set_link();

    bool is_ready();
    void out(int64_t user_id);
    bool is_alive(int64_t user_id);
    std::string get_ng(int64_t user_id);
    int get_alive_num();
    int get_player_num();

    std::vector<int64_t> get_player_list();
    gameState get_state();
    void update(bool toabort);

    std::string get_info(int64_t user_id);
};

class NGgame : public processable
{
  public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
};
