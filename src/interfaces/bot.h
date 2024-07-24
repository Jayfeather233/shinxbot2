#pragma once

#define QQBOT

#include <iostream>
#include <jsoncpp/json/json.h>

enum LOG { INFO = 0, WARNING = 1, ERROR = 2 };
extern std::string LOG_name[];

class bot;

typedef uint64_t userid_t;
typedef uint64_t groupid_t;

/**
 * message_type: "group" or "private"
 * user_id & group_id
 * message_id
 * p
 */
struct msg_meta {
    std::string message_type;
    userid_t user_id;
    groupid_t group_id;
    int64_t message_id;
    bot *p;

    msg_meta(const msg_meta &u);
    msg_meta(const msg_meta &&u);
    msg_meta(std::string mt="", userid_t uid=0, groupid_t gid=0, int64_t mid=0, bot*pp=nullptr);
};

class bot {
protected:
    int receive_port, send_port;

    userid_t botqq;

public:
    bot() = delete;
    bot(bot &bot) = delete;
    bot(bot &&bot) = delete;
    bot(int recv_port, int send_port);

    /**
     * Starter of the bot.
     * ATTENTION: run() must not quit. Otherwise the main() will call it again.
     */
    virtual void run() = 0;

    /**
     * Is this user the bot's operator?
     */
    virtual bool is_op(const userid_t a) const;

    /**
     * send(POST) to gocq (or any other you want)
     */
    virtual std::string cq_send(const std::string &message,
                                const msg_meta &conf) const;
    
    virtual void cq_send_all_op(const std::string &message) = 0;

    /**
     * send(POST) to gocq (or any other you want)
     */
    virtual std::string cq_send(const std::string &end_point,
                                const Json::Value &J) const;

    /**
     * GET from gocq (or any other you want)
     */
    virtual std::string cq_get(const std::string &end_point) const;

    /**
     * Write down logs.
     */
    virtual void setlog(LOG type, std::string message);

    /**
     * get mine qq. (or other id-like-thing)
     */
    virtual userid_t get_botqq() const;

    /**
     * receive a message, how to process
     */
    virtual void input_process(std::string *input) = 0;

    virtual ~bot();
};