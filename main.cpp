#include "utils.h"
#include "processable.h"
#include "eventprocess.h"
#include "functions.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <thread>

int send_port = 5750;
int recieve_port = 5701;

std::string LOG_name[3] = {"INFO", "WARNING", "ERROR"};

std::vector<processable*> functions;
std::vector<eventprocess*> events;

void input_process(std::string *input){
    Json::Value J = string_to_json(*input);
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
                long user_id = 0, group_id = -1;
                if(J.isMember("group_id")){
                    group_id = J["group_id"].asInt64();
                }
                if(J.isMember("user_id")){
                    user_id = J["user_id"].asInt64();
                }
                if (message == "bot.help") {
                    std::string help_message;
                    for (processable *game : functions) {
                        if (game->help() != "")
                            help_message += game->help() + '\n';
                    }
                    help_message += "本Bot项目地址：https://github.com/Jayfeather233/shinxBot";
                    cq_send(help_message, message_type, user_id, group_id);
                } else {
                    for (processable *game : functions) {
                        if (game->check(message, message_type, user_id, group_id)) {
                            game->process(message, message_type, user_id, group_id);
                        }
                    }
                }
            }
        }
    }
    delete input;
}

void handle_accept(boost::asio::ip::tcp::socket &socket){
    boost::array<char, 4096> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);
    
    std::istringstream data (buf.c_array());
    std::string line;    
    while (std::getline(data, line)) {
        if(line[0]=='{'){
            //input_process(line);
            std::string *u = new std::string(line);
            std::thread th = std::thread(input_process, u);
            th.detach();
        }
    }
    
    std::string res_header = 
                    (std::string)"HTTP/1.1 200 OK\n"+
                    "Content-Length: 0\n"+
                    "Content-Type: application/json\n"+ "\n"
                    ;
    boost::asio::write(socket, boost::asio::buffer(res_header));
    socket.close();
}

int main(){
    username_init();
    functions.push_back(new AnimeImg());

    try {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), recieve_port));
        for (;;) {
            boost::asio::ip::tcp::socket socket(io_service);
            acceptor.accept(socket);
            handle_accept(socket);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

std::string cq_send(std::string message, std::string message_type, long user_id, long group_id){
    Json::Value input;
    input["message"] = message;
    input["message_type"] = message_type;
    input["group_id"] = group_id;
    input["user_id"] = user_id;
    return cq_send("send_msg", input);
}

std::string cq_send(std::string end_point, Json::Value J){
    return do_post(("http://127.0.0.1:" + std::to_string(send_port) + "/" + end_point).c_str(), J);
}

std::mutex mylock;

void setlog(LOG type, std::string message){
    std::lock_guard<std::mutex> lock(mylock);
    std::cout<<"[" << LOG_name[type] << "] " << message <<std::endl;
}