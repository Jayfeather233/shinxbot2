#pragma once

// #include "events.h"
// #include "functions.h"
#include "eventprocess.h"
#include "heartbeat.h"
#include "processable.h"

#include <fstream>
#include <mutex>
#include <unistd.h>
#include <vector>

class mybot : public bot {
private:
    bool bot_is_on = true;

    std::ofstream LOG_output[3];
    std::mutex log_lock;
    tm last_getlog;

    // pointer, handler, name
    std::vector<std::tuple<processable *, void *, std::string>> functions;
    std::vector<std::tuple<eventprocess *, void *, std::string>> events;
    std::set<userid_t> op_list;

    bool bot_isopen = true;

    heartBeat *recorder;
    Timer *mytimer;
    archivist *archive;

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

    /**
     * Handle some meta_event start with 'bot.'
     */
    bool meta_func(std::string message, const msg_meta &conf);
    void unload_func(std::tuple<processable *, void *, std::string> &f);
    void unload_func(std::tuple<eventprocess *, void *, std::string> &f);
    
    void init_func(const std::string &name, processable *p);
    void init_func(const std::string &name, eventprocess *p);

public:
    mybot(int recv_port, int send_port);

    bool is_op(const userid_t a) const;

    void input_process(std::string *input);

    void run();

    void setlog(LOG type, std::string message);
    
    void cq_send_all_op(const std::string &message);

    ~mybot();
};