#pragma once

#include "events.h"
#include "functions.h"

#include <fstream>
#include <mutex>
#include <unistd.h>
#include <vector>

class mybot : public bot {
private:
    bool bot_is_on = true;
    int64_t botqq;

    std::ofstream LOG_output[3];
    std::mutex log_lock;
    tm last_getlog;

    std::vector<processable *> functions;
    std::vector<eventprocess *> events;
    std::set<int64_t> op_list;

    bool bot_isopen = true;

    /**
     * after connect to gocq, read the message out
     */
    void read_server_message(int new_socket);

    /**
     * TCP connect with gocq to establish connection
     */
    int start_server();

    /**
     * Check if the log output stream should be updated
     */
    void log_init();

    /**
     * Get bot's qq and read op_list
     */
    void init();

public:
    mybot(int recv_port, int send_port);

    bool is_op(const int64_t a) const;

    void input_process(std::string *input);

    void run();

    void setlog(LOG type, std::string message);

    ~mybot();
};