#pragma once

#include <iostream>
#include <jsoncpp/json/json.h>

enum LOG { INFO=0, WARNING=1, ERROR=2 };
extern std::string LOG_name[];

class bot;

/**
 * message_type: "group" or "private"
 * user_id & group_id
 * message_id
 * p
 */
struct msg_meta {
    std::string message_type;
    int64_t user_id;
    int64_t group_id;
    int64_t message_id;
    bot *p;
};

class bot{
protected:
    int receive_port, send_port;
public:
    bot() = delete;
    bot(bot &bot) = delete;
    bot(bot &&bot) = delete;
    bot(int recv_port, int send_port) : receive_port(recv_port), send_port(send_port){}
    virtual void run() = 0;
    virtual bool is_op(const int64_t a) const = 0;

    virtual std::string cq_send(const std::string &message, const msg_meta &conf) const = 0;
    virtual std::string cq_send(const std::string &end_point, const Json::Value &J) const = 0;
    virtual std::string cq_get(const std::string &end_point) const = 0;
    virtual void setlog(LOG type, std::string message) = 0;
    virtual int64_t get_botqq() const = 0;
    virtual void input_process(std::string *input) = 0;
};