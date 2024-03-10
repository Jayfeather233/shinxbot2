#include "mybot.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

void mybot::read_server_message(int new_socket)
{
    char buffer[4096];
    try {
        std::string s_buffer;
        int valread;
        while (1) {
            valread = read(new_socket, buffer, 4000);
            if (valread < 0) {
                break;
            }
            buffer[valread] = 0;
            s_buffer += buffer;
            if (valread < 4000) {
                break;
            }
        }
        if (valread == -1) {
            setlog(LOG::ERROR, "Error read message.");
        }

        std::istringstream iss(s_buffer);
        std::string msg, line;
        bool flg = false;
        while (std::getline(iss, line)) {
            if (line[0] == '{') {
                flg = true;
            }
            if (flg)
                msg += line;
        }
        std::string *u = new std::string(msg);
        std::thread(&mybot::input_process, this, u).detach();

        std::stringstream response_body;
        response_body << "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 0\r\n"
                         "Content-Type: application/json\r\n\r\n";
        std::string response = response_body.str();
        const char *response_cstr = response.c_str();
        send(new_socket, response_cstr, strlen(response_cstr), 0);
    }
    catch (...) {
    }
    close(new_socket);
}

int mybot::start_server()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        setlog(LOG::ERROR, "Error creating socket");
        return 1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        setlog(LOG::ERROR, "Error setting socket options");
        return 1;
    }

    // Set server address and bind to socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(receive_port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        setlog(LOG::ERROR, "Error binding to socket");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        setlog(LOG::ERROR, "Error listening for connections");
        return 1;
    }

    // Accept incoming connections and handle requests
    while (bot_is_on) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0) {
            setlog(LOG::ERROR, "Error accepting connection");
            continue;
        }
        read_server_message(new_socket);
    }
    return 0;
}
void mybot::log_init()
{
    std::ostringstream oss;
    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *std::localtime(
        &nt); // No need to delete according to
              // [stackoverflow](https://stackoverflow.com/questions/64854691/do-i-need-to-delete-the-pointer-returned-by-stdlocaltime)
              // (?)

    oss << "./log/" << botqq << "/" << tt.tm_year + 1900 << '_' << std::setw(2)
        << std::setfill('0') << tt.tm_mon + 1 << '_' << std::setw(2)
        << std::setfill('0') << tt.tm_mday;

    if (!std::filesystem::exists(oss.str().c_str())) {
        std::filesystem::create_directories(oss.str().c_str());
    }
    for (int i = 0; i < 3; i++) {
        if (LOG_output[i].is_open()) {
            LOG_output[i].close();
        }
    }
    LOG_output[0] = std::ofstream(oss.str() + "/info.log", std::ios_base::app);
    LOG_output[1] = std::ofstream(oss.str() + "/warn.log", std::ios_base::app);
    LOG_output[2] = std::ofstream(oss.str() + "/erro.log", std::ios_base::app);
}

void mybot::init()
{
    for (;;) {
        try {
            Json::Value J = string_to_json(cq_get("get_login_info"));
            botqq = J["data"]["user_id"].asInt64();
            break;
        }
        catch (...) {
        }
        sleep(10); // 10 sec
    }
    std::cout << "botqq:" << botqq << std::endl;

    log_init();

    Json::Value J_op = string_to_json(readfile("./config/op_list.json", "[]"));
    parse_json_to_set(J_op, op_list);

    Json::Value J_rec = string_to_json(readfile("./config/recover.json", "[]"));
    Json::Value Ja_rec = J_rec["commands"];
    Json::ArrayIndex sz = Ja_rec.size();
    std::vector<std::string> rec_list;
    for(Json::ArrayIndex i = 0; i < sz; ++i){
        rec_list.push_back(Ja_rec[i].asString());
    }

    recorder = new heartBeat(rec_list);
}

mybot::mybot(int recv_port, int send_port) : bot(recv_port, send_port) {}

bool mybot::is_op(const int64_t a) const
{
    return op_list.find(a) != op_list.end();
}

void mybot::input_process(std::string *input)
{
    Json::Value J = string_to_json(*input);
    delete input;
    std::string post_type = J["post_type"].asString();

    if ((post_type == "request" || post_type == "notice") && bot_isopen) {
        for (eventprocess *even : events) {
            if (even->check(this, J)) {
                even->process(this, J);
            }
        }
    }
    else if (post_type == "message") {
        if (J.isMember("message_type") && J.isMember("message")) {
            std::string message = J["message"].asString();
            int64_t message_id = J["message_id"].asInt64();
            std::string message_type = J["message_type"].asString();
            if (message_type == "group" || message_type == "private") {
                int64_t user_id = 0, group_id = -1;
                if (J.isMember("group_id")) {
                    group_id = J["group_id"].asInt64();
                }
                if (J.isMember("user_id")) {
                    user_id = J["user_id"].asInt64();
                }
                msg_meta conf = (msg_meta){message_type, user_id, group_id,
                                           message_id, this};
                if (message == "bot.help") {
                    std::string help_message;
                    for (processable *func : functions) {
                        if (func->help() != "")
                            help_message += func->help() + '\n';
                    }
                    help_message += "本Bot项目地址：https://github.com/"
                                    "Jayfeather233/shinxbot2";
                    cq_send(help_message, conf);
                }
                else if (message == "bot.off" && is_op(user_id)) {
                    bot_isopen = false;
                    cq_send("isopen=" + std::to_string(bot_isopen), conf);
                }
                else if (message == "bot.on" && is_op(user_id)) {
                    bot_isopen = true;
                    cq_send("isopen=" + std::to_string(bot_isopen), conf);
                }
                else if (bot_isopen) {
                    for (processable *func : functions) {
                        if (func->check(message, conf)) {
                            try {
                                func->process(message, conf);
                            }
                            catch (char *e) {
                                cq_send((std::string) "Throw an char*: " + e,
                                        conf);
                                setlog(LOG::ERROR,
                                       (std::string) "Throw an char*: " + e);
                            }
                            catch (std::string e) {
                                cq_send("Throw an string: " + e, conf);
                                setlog(LOG::ERROR, "Throw an string: " + e);
                            }
                            catch (std::exception &e) {
                                cq_send((std::string) "Throw an exception: " +
                                            e.what(),
                                        conf);
                                setlog(LOG::ERROR,
                                       (std::string) "Throw an exception: " +
                                           e.what());
                            }
                            catch (...) {
                                cq_send("Throw an unknown error", conf);
                                setlog(LOG::ERROR, "Throw an unknown error");
                            }
                        }
                    }
                }
            }
        }
    } else if(post_type == "meta_event"){
        if(J["meta_event_type"].asString() == "heartbeat"){
            recorder->inform();
        }
    }
}

void mybot::run()
{
    functions.push_back(new AnimeImg());
    functions.push_back(new auto114());
    functions.push_back(new hhsh());
    functions.push_back(new fudu());
    functions.push_back(new forward_msg_gen());
    functions.push_back(new r_color());
    functions.push_back(new catmain());
    functions.push_back(new ocr());
    functions.push_back(new httpcats());
    functions.push_back(new gpt3_5()); // Sorry but no keys
    functions.push_back(new img());
    functions.push_back(new recall());
    // functions.push_back(new forwarder()); // Easy to get your account frozen.
    functions.push_back(new gray_list());
    functions.push_back(new original());
    functions.push_back(new bili_decode());
    functions.push_back(new gemini());
    functions.push_back(new sdxl());
    functions.push_back(new img_fun());

    events.push_back(new talkative());
    events.push_back(new m_change());
    events.push_back(new friendadd());
    events.push_back(new poke());

    this->init();

    msg_meta start_msg_conf;
    start_msg_conf.message_type = "private";

    for (int64_t ops : op_list) {
        start_msg_conf.user_id = ops;
        this->cq_send("Love you!", start_msg_conf);
    }
    std::thread(&heartBeat::run, recorder).detach();

    this->start_server();

    for (processable *u : functions) {
        delete u;
    }
    for (eventprocess *u : events) {
        delete u;
    }
}

void mybot::setlog(LOG type, std::string message)
{
    std::lock_guard<std::mutex> lock(log_lock);

    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);

    if (!(tt.tm_year == last_getlog.tm_year &&
          tt.tm_mon == last_getlog.tm_mon &&
          tt.tm_mday == last_getlog.tm_mday)) {
        last_getlog = tt;
        this->log_init();
    }

    std::ostringstream oss;
    oss << "[" << std::setw(2) << std::setfill('0') << tt.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tt.tm_min << ":" << std::setw(2)
        << std::setfill('0') << tt.tm_sec << "][" << LOG_name[type] << "] "
        << message << std::endl;

    if (type == LOG::ERROR)
        std::cerr << oss.str();
    else
        std::cout << oss.str();
    LOG_output[type] << oss.str();
    LOG_output[type].flush();
}

mybot::~mybot() { bot_is_on = false; }