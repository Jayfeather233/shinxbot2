#include "events.h"
#include "functions.h"
#include "utils.h"

#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

int send_port;
int receive_port;

int64_t botqq;

std::string LOG_name[3] = {"INFO", "WARNING", "ERROR"};
std::ofstream LOG_output[3];

std::vector<processable *> functions;
std::vector<eventprocess *> events;
std::set<int64_t> op_list;

void input_process(std::string *input)
{
    Json::Value J = string_to_json(*input);
    delete input;
    std::string post_type = J["post_type"].asString();

    if (post_type == "request" || post_type == "notice") {
        for (eventprocess *even : events) {
            if (even->check(J)) {
                even->process(J);
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
                msg_meta conf =
                    (msg_meta){message_type, user_id, group_id, message_id};
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
                else {
                    for (processable *func : functions) {
                        if (func->check(message, conf)) {
                            try {
                                func->process(message, conf);
                            }
                            catch (char *e) {
                                cq_send((std::string) "Throw an char*: " + e,
                                        conf);
                            }
                            catch (std::string e) {
                                cq_send("Throw an string: " + e, conf);
                            }
                            catch (std::exception &e) {
                                cq_send((std::string) "Throw an exception: " +
                                            e.what(),
                                        conf);
                            }
                            catch (...) {
                                cq_send("Throw an unknown error", conf);
                            }
                        }
                    }
                }
            }
        }
    }
}

void read_server_message(int new_socket)
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
        std::thread(input_process, u).detach();

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

int start_server()
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
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0) {
            setlog(LOG::ERROR, "Error accepting connection");
            continue;
        }
        read_server_message(new_socket);
    }
    return 0;
}

void get_log()
{
    std::ostringstream oss;
    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);

    oss << tt.tm_year + 1900 << '_' << std::setw(2) << std::setfill('0')
        << tt.tm_mon + 1 << '_' << std::setw(2) << std::setfill('0')
        << tt.tm_mday;

    if (!std::filesystem::exists(("./log/" + oss.str()).c_str())) {
        if (!std::filesystem::exists("./log")) {
            std::filesystem::create_directory("./log");
        }
        std::filesystem::create_directory(("./log/" + oss.str()).c_str());
    }
    LOG_output[0] =
        std::ofstream("./log/" + oss.str() + "/info.log", std::ios_base::app);
    LOG_output[1] =
        std::ofstream("./log/" + oss.str() + "/warn.log", std::ios_base::app);
    LOG_output[2] =
        std::ofstream("./log/" + oss.str() + "/erro.log", std::ios_base::app);
}

bool is_op(const int64_t &a) { return op_list.find(a) != op_list.end(); }

void init()
{
    std::ifstream iport("./config/port.txt");
    if (iport.is_open()) {
        iport >> send_port >> receive_port;
        iport.close();
    }
    else {
        std::cout << "Please input the send_port: (receive port in go-cqhttp):";
        std::cin >> send_port;
        std::cout << "Please input the receive_port: (send port in go-cqhttp):";
        std::cin >> receive_port;
        std::ofstream oport("./config/port.txt");
        if (oport) {
            oport << send_port << ' ' << receive_port;
            oport.flush();
            oport.close();
        }
    }

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

    get_log();

    Json::Value J_op = string_to_json(readfile("./config/op_list.json", "[]"));
    parse_json_to_set(J_op, op_list);
}

int main()
{
    curl_global_init(CURL_GLOBAL_ALL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);

    functions.push_back(new AnimeImg());
    functions.push_back(new auto114());
    functions.push_back(new hhsh());
    functions.push_back(new fudu());
    functions.push_back(new forward());
    functions.push_back(new r_color());
    functions.push_back(new e621());
    functions.push_back(new catmain());
    functions.push_back(new ocr());
    functions.push_back(new httpcats());
    functions.push_back(new gpt3_5());
    functions.push_back(new img());
    functions.push_back(new recall());
    functions.push_back(new forwarder());
    functions.push_back(new gray_list());

    events.push_back(new talkative());
    events.push_back(new m_change());
    events.push_back(new friendadd());
    events.push_back(new poke());

    init();

    msg_meta start_msg_conf;
    start_msg_conf.message_type = "private";

    for (int64_t ops : op_list) {
        start_msg_conf.user_id = ops;
        cq_send("bot start.", start_msg_conf);
    }

    start_server();

    for (processable *u : functions) {
        delete u;
    }
    for (eventprocess *u : events) {
        delete u;
    }
    curl_global_cleanup();

    return 0;
}

std::string cq_send(const std::string &message, const msg_meta &conf)
{
    Json::Value input;
    input["message"] = message;
    input["message_type"] = conf.message_type;
    input["group_id"] = conf.group_id;
    input["user_id"] = conf.user_id;
    return cq_send("send_msg", input);
}

std::string cq_send(const std::string &end_point, const Json::Value &J)
{
    return do_post("127.0.0.1:" + std::to_string(send_port) + "/" + end_point,
                   J);
}
std::string cq_get(const std::string &end_point)
{
    return do_get("127.0.0.1:" + std::to_string(send_port) + "/" + end_point);
}

std::mutex mylock;

tm last_getlog;

void setlog(LOG type, std::string message)
{
    std::lock_guard<std::mutex> lock(mylock);

    std::time_t nt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);

    if (!(tt.tm_year == last_getlog.tm_year &&
          tt.tm_mon == last_getlog.tm_mon &&
          tt.tm_mday == last_getlog.tm_mday)) {
        last_getlog = tt;
        get_log();
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

int64_t get_botqq() { return botqq; }

std::string get_local_path() { return std::filesystem::current_path(); }
