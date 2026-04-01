#pragma once

#include "eventprocess.h"
#include "heartbeat.h"
#include "processable.h"

#include <fstream>
#include <mutex>
#include <thread>
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
    // ===== Runtime flags/state =====
    bool bot_enabled = true;

    // ===== Logging =====
    std::ofstream LOG_output[3];
    std::mutex log_lock;
    tm last_getlog{};

    // ===== Dynamic module handles (pointer, handler, alias) =====
    std::vector<std::tuple<processable *, void *, std::string>> functions;
    std::vector<std::tuple<eventprocess *, void *, std::string>> events;
    std::set<userid_t> op_list;

    // ===== Core services =====
    heartBeat *recorder = nullptr;
    Timer *mytimer = nullptr;
    archivist *archive = nullptr;

    // ===== module_load desired state =====
    std::set<std::string> enabled_functions;
    std::set<std::string> enabled_events;

    // ===== Group policy state =====
    std::map<groupid_t, blockItem> group_blocklist;
    std::thread heartbeat_thread;

    // ===== Group policy helpers =====
    void save_blocklist();

    // ===== module_load helpers =====
    void load_module_filter_config();
    void save_module_filter_config() const;
    void add_module_to_filter(const std::string &name, bool is_event);
    void remove_module_from_filter(const std::string &name, bool is_event);
    std::vector<std::string> list_available_module_names(bool is_event) const;

    // ===== OP command handlers =====
    bool handle_bot_load(const std::string &message, const msg_meta &conf);
    bool handle_bot_unload(const std::string &message, const msg_meta &conf);
    bool handle_bot_reload(const std::string &message, const msg_meta &conf);
    bool meta_func(std::string message, const msg_meta &conf);

    // ===== Networking/log bootstrap =====
    int start_server();
    void refresh_log_stream();

    // ===== Lifecycle internals =====
    void init();
    void unload_func(std::tuple<processable *, void *, std::string> &f);
    void unload_func(std::tuple<eventprocess *, void *, std::string> &f);
    void init_func(const std::string &name, processable *p);
    void init_func(const std::string &name, eventprocess *p);

public:
    // ===== Construction =====
    shinxbot(int recv_port, int send_port, const std::string &tk);
    shinxbot(const Json::Value &J);

    // ===== Entry points =====
    bool is_op(const userid_t a) const;
    void input_process(const std::string &input);
    void run();
    void setlog(LOG type, std::string message);
    void cq_send_all_op(const std::string &message);

    // ===== Teardown =====
    ~shinxbot();
};