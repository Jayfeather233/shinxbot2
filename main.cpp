#include "utils.h"
#include "functions.h"
#include "events.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

int send_port;
int receive_port;

int64_t botqq;

std::string LOG_name[3] = {"INFO", "WARNING", "ERROR"};
std::ofstream LOG_output[3];

std::vector<processable*> functions;
std::vector<eventprocess*> events;

void input_process(std::string *input){
    Json::Value J = string_to_json(*input);
    delete input;
    std::string post_type = J["post_type"].asString();

    if(post_type == "request" || post_type == "notice"){
        for (eventprocess *even : events) {
            if (even->check(J)) {
                even->process(J);
            }
        }
    } else if(post_type == "message"){
        if(J.isMember("message_type") && J.isMember("message")){
            std::string message = J["message"].asString();
            int message_id = J["message_id"].asInt();
            std::string message_type = J["message_type"].asString();
            if(message_type == "group" || message_type == "private"){
                int64_t user_id = 0, group_id = -1;
                if(J.isMember("group_id")){
                    group_id = J["group_id"].asInt64();
                }
                if(J.isMember("user_id")){
                    user_id = J["user_id"].asInt64();
                }
                if (message == "bot.help") {
                    std::string help_message;
                    for (processable *func : functions) {
                        if (func->help() != "")
                            help_message += func->help() + '\n';
                    }
                    help_message += "本Bot项目地址：https://github.com/Jayfeather233/shinxbot2";
                    cq_send(help_message, message_type, user_id, group_id);
                } else {
                    for (processable *func : functions) {
                        if (func->check(message, message_type, user_id, group_id)) {
                            func->process(message, message_type, user_id, group_id);
                        }
                    }
                }
            }
        }
    }
}


int start_server(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[10240] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Error setting socket options\n";
        return 1;
    }

    // Set server address and bind to socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(receive_port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding to socket\n";
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Error listening for connections\n";
        return 1;
    }

    // Accept incoming connections and handle requests
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }
        int valread = read(new_socket, buffer, 10230);
        if(valread == -1){
            std::cerr<<"Error read message.\n";
        }

        std::istringstream iss((std::string)buffer);
        std::string line;
        while(getline(iss, line)){
            if(line[0]=='{'){
                std::string *u = new std::string(line);
                std::thread(input_process, u).detach();
            }
        }

        std::stringstream response_body;
        response_body << (std::string)"HTTP/1.1 200 OK\r\n"+
                    "Content-Length: 0\r\n"+
                    "Content-Type: application/json\r\n\n";
        std::string response = response_body.str();
        const char* response_cstr = response.c_str();
        send(new_socket, response_cstr, strlen(response_cstr), 0);
        close(new_socket);
    }
    return 0;
}

void init(){

    std::ifstream iport("./config/port.txt");
    if(iport.is_open()){
        iport >> send_port >> receive_port;
        iport.close();
    } else {
        std::cout<<"Please input the send_port: (receive port in go-cqhttp):";
        std::cin>>send_port;
        std::cout<<"Please input the receive_port: (send port in go-cqhttp):";
        std::cin>>receive_port;
        std::ofstream oport("./config/port.txt");
        if(oport){
            oport << send_port << ' ' << receive_port;
            oport.flush();oport.close();
        }
    }

    for(;;){
        try{
            Json::Value J = string_to_json(cq_get("get_login_info"));
            botqq = J["data"]["user_id"].asInt64();
            break;
        } catch (...) {

        }
        sleep(10);//10 sec
    }
    std::cout<<"botqq:"<<botqq<<std::endl;

    std::ostringstream oss;
    std::time_t nt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);

    oss << tt.tm_year + 1900 << '_' << tt.tm_mon + 1 << '_' << tt.tm_mday;

    if(!std::filesystem::exists(("./log/" + oss.str()).c_str())){
        if(!std::filesystem::exists("./log")){
            std::filesystem::create_directory("./log");
        }
        std::filesystem::create_directory(("./log/" + oss.str()).c_str());
    }
    LOG_output[0] = std::ofstream("./log/" + oss.str() + "/info.log", std::ios_base::app);
    LOG_output[1] = std::ofstream("./log/" + oss.str() + "/warn.log", std::ios_base::app);
    LOG_output[2] = std::ofstream("./log/" + oss.str() + "/erro.log", std::ios_base::app);
}

int main(){
    curl_global_init(CURL_GLOBAL_ALL);

    init();

    functions.push_back(new AnimeImg());
    functions.push_back(new auto114());
    functions.push_back(new hhsh());
    functions.push_back(new fudu());
    functions.push_back(new forward());
    functions.push_back(new r_color());
    functions.push_back(new e621());
    functions.push_back(new catmain());

    events.push_back(new talkative());
    events.push_back(new m_change());
    events.push_back(new friendadd());
    events.push_back(new poke());

    start_server();

    for(processable *u : functions){
        delete []u;
    }
    for(eventprocess *u : events){
        delete []u;
    }
    curl_global_cleanup();

    return 0;
}

std::string cq_send(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    Json::Value input;
    input["message"] = message;
    input["message_type"] = message_type;
    input["group_id"] = group_id;
    input["user_id"] = user_id;
    return cq_send("send_msg", input);
}

std::string cq_send(std::string end_point, Json::Value J){
    std::string re = do_post("127.0.0.1:" + std::to_string(send_port) + "/" + end_point, J);
    return re;
}
std::string cq_get(std::string end_point){
    std::string re = do_get("127.0.0.1:" + std::to_string(send_port) + "/" + end_point);
    return re;
}

std::mutex mylock;

void setlog(LOG type, std::string message){
    std::time_t nt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tt = *localtime(&nt);

    std::lock_guard<std::mutex> lock(mylock);

    std::ostringstream oss;
    oss << "[" << tt.tm_hour << ":" << tt.tm_min << ":"<< tt.tm_sec << "][" << LOG_name[type] << "] "
    << message <<std::endl;

    std::cout<< oss.str();
    LOG_output[type] << oss.str();
    LOG_output[type].flush();
}

int64_t get_botqq(){
    return botqq;
}

std::string get_local_path(){
    return std::filesystem::current_path();
}