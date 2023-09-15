#include <arpa/inet.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int send_port_bot;
int receive_port_bot;

// https://stackoverflow.com/a/26221725/17792535
template <typename... Args>
std::string string_format(const std::string &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
                 1; // Extra space for '\0'
    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}

void get_config()
{
    std::ifstream iport("../../config/port.txt");
    if (iport.is_open()) {
        iport >> send_port_bot >> receive_port_bot;
        std::cout << send_port_bot << " " << receive_port_bot << std::endl;
        iport.close();
    }
    else {
        std::cerr << "Please first generate port.txt by running `cq_bot`"
                  << std::endl;
        std::exit(1);
    }
}

std::string process(const std::string request) { 
    std::string response = "{}";
    std::istringstream iss{request};
    std::string newline;
    std::getline(iss, newline);
    if(newline.find("/get_login_info") != std::string::npos){
       response = "{\"user_id\": 23333, \"nickname\":\"Test\"}";
    }
    std::string cq_body = R"({
        "status": "ok",
        "retcode": 0,
        "msg": "",
        "wording": "",
        "data": %s,
        "echo": ""
    })";
    cq_body = string_format(cq_body, response.c_str());
    std::cout << "RESPONSE: " << cq_body << std::endl << std::endl;
    return cq_body;
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
                std::cerr << "Error read message.\n";
            }
            buffer[valread] = 0;
            s_buffer += buffer;
            if (valread < 4000) {
                break;
            }
        }

        std::cout << s_buffer << std::endl;

        std::string body{process(s_buffer)};
        std::string head = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: %d\r\n"
                           "Content-Type: application/json\r\n\r\n";
        head = string_format(head, body.length());
        std::string response = head + body;

        send(new_socket, response.c_str(), response.length(), 0);
    }
    catch (const std::exception &exc) {
        std::cerr << exc.what();
    }
    close(new_socket);
}

int start_server()
{
    get_config();
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    // char buffer[4096] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        std::cerr << "Error setting socket options\n";
        return 1;
    }

    // Set server address and bind to socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(send_port_bot);
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
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }
        // std::thread(read_server_message, new_socket).detach();
        read_server_message(new_socket);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    start_server();
    return 0;
}
