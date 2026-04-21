#include "luck.h"
#include "utils.h"
#include <chrono>
#include <random>

void luck::process(std::string message, const msg_meta &conf)
{

    auto now = std::chrono::system_clock::now();
    auto beijing_now = now + std::chrono::hours(8);
    std::time_t now_c = std::chrono::system_clock::to_time_t(beijing_now);
    std::tm *now_tm = std::gmtime(&now_c);

    int year = now_tm->tm_year + 1900;
    int month = now_tm->tm_mon + 1;
    int day = now_tm->tm_mday;

    uint64_t date_int = year * 10000 + month * 100 + day;
    std::mt19937 rng(conf.user_id + date_int);
    std::uniform_int_distribution<int> dist(1, 100);
    int luck_val = dist(rng);

    std::string reply = "[CQ:reply,id=" + std::to_string(conf.message_id) +
                        "]你今天的人品值是" + std::to_string(luck_val);

    conf.p->cq_send(reply, conf);

    conf.p->setlog(LOG::INFO, "luck_module: user " +
                                  std::to_string(conf.user_id) + " got " +
                                  std::to_string(luck_val));
}

bool luck::check(std::string message, const msg_meta &conf)
{
    (void)conf;

    return cmd_match_exact(trim(message), {
                                              "jrrp",
                                              ".jrrp",
                                              "今日人品",
                                              ".今日人品",
                                          });
}

std::string luck::help() { return "今日人品：查看今天的人品值"; }

DECLARE_FACTORY_FUNCTIONS(luck)
