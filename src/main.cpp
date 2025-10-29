#include "utils.h"
#include "shinxbot.hpp"

#include <vector>
#include <atomic>
#include <sys/wait.h>
#include <thread>

#include <Magick++.h>

int send_port, receive_port;
bot* bots;

void bot_run(bot *u){
    while(1){
        int k = fork();
        if(k==-1){
            std::cerr<< "Process Error!"<<std::endl;
        } else if(k==0){
            u->run();
            exit(0);
        } else {
            waitpid(k, NULL, 0);
        }
    }
}

int main()
{
    Magick::InitializeMagick("shinxBot");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    std::ifstream iport("./config/port.txt");
    int x, y;
    std::string token;
    if (iport.is_open()) {
        while(!iport.eof()){
            iport >> x >> y >> token;
        }
        iport.close();
    }
    else {
        std::cout << "Please input the send_port: (receive port in Onebot11):";
        std::cin >> x;
        std::cout << "Please input the receive_port: (send port in Onebot11):";
        std::cin >> y;
        std::cout << "Please input the token:";
        std::cin >> token;
        std::ofstream oport("./config/port.txt");
        if (oport) {
            oport << x << ' ' << y << ' ' << token;
            oport.flush();
            oport.close();
        }
    }

    bot_run(bots = new shinxbot(y, x, token));
    
    while(true) sleep(10);

    //Never goes here~
    delete bots;

    Magick::TerminateMagick();

    return 0;
}
