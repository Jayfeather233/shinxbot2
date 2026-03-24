#pragma once

#include "eventprocess.h"
#include "heartbeat.h"
#include "processable.h"

#include <fstream>
#include <mutex>
#include <unistd.h>
#include <vector>

class blockItem {
private:
    std::set<std::string> blocklist;
    std::set<std::string> whitelist;
    bool mode; // true: blocklist, false: whitelist
public:
    blockItem() : mode(true) {}
    blockItem(const std::set<std::string> &blocklist,
              const std::set<std::string> &whitelist, bool mode)
        : blocklist(blocklist), whitelist(whitelist), mode(mode)
    {
    }
    blockItem(const Json::Value &J)
    {
        if (J.isMember("block") && J.isMember("white") && J.isMember("mode")) {
            parse_json_to_set(J["block"], blocklist);
            parse_json_to_set(J["white"], whitelist);
            mode = J["mode"].asBool();
        }
        else {
            mode = true;
        }
    }
    bool is_blocked(const std::string &message)
    {
        if (mode) {
            return blocklist.find(message) != blocklist.end();
        }
        else {
            return whitelist.find(message) == whitelist.end();
        }
    }
    void add_block(const std::string &message)
    {
        blocklist.insert(message);
        mode = true;
    }
    void remove_block(const std::string &message)
    {
        blocklist.erase(message);
        mode = true;
    }
    void add_white(const std::string &message)
    {
        whitelist.insert(message);
        mode = false;
    }
    void remove_white(const std::string &message)
    {
        whitelist.erase(message);
        mode = false;
    }
    void clear()
    {
        blocklist.clear();
        whitelist.clear();
        mode = true;
    }
    Json::Value to_json() const
    {
        Json::Value J;
        J["block"] = parse_set_to_json(blocklist);
        J["white"] = parse_set_to_json(whitelist);
        J["mode"] = mode;
        return J;
    }
};

class shinxbot : public bot {
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

    std::map<groupid_t, blockItem> group_blocklist;
    void save_blocklist();

    /**
     * after connect to gocq, read the message out
     */
    void read_server_message(int new_socket);

    /**
     * TCP connect with gocq to establish connection
     */
    int start_server();

    /**
     * refresh the log output stream should be updated
     */
    void refresh_log_stream();

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
    shinxbot(int recv_port, int send_port, std::string tk);
    shinxbot(const Json::Value &J);

    bool is_op(const userid_t a) const;

    void input_process(std::string *input);

    void run();

    void setlog(LOG type, std::string message);

    void cq_send_all_op(const std::string &message);

    ~shinxbot();
};