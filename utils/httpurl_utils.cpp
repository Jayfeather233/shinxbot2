#include "utils.h"

std::pair<std::string, std::string> divide_http_addr(const std::string &url){
    size_t pos = 1;
    if(url.find("http://") != std::string::npos){
        pos = url.find("/", 8);
    } else if(url.find("https://") != std::string::npos){
        pos = url.find("/", 9);
    } else {
        pos = url.find("/");
    }
    if(pos == std::string::npos){
        return std::make_pair(url, "");
    } else {
        return std::make_pair(url.substr(0, pos), url.substr(pos+1));
    }
}