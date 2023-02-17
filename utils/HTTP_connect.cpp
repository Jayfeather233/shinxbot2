#include "utils.h"

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    ((std::string*)userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string do_post(const char* url, Json::Value json_message){
    CURL *curl;
    CURLcode res;
    
    std::string string_message = json_message.toStyledString();
    std::string response;
    std::string method = "POST";

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, string_message.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, string_message.size());
        
        curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) 
                << std::endl;
            throw "http failed";
        } else {
            return response;
        }
    } else {
        std::cerr << "curl_easy_init() failed: " << std::endl;
        throw "curl failed";
    }
}

std::string do_get(const char* url){
    CURL *curl;
    CURLcode res;
    
    std::string response;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, curl_easy_escape(curl, url, 0));
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) 
                << std::endl;
            return "";
        } else {
            return response;
        }
    } else {
        std::cerr << "curl_easy_init() failed: " << std::endl;
        return "";
    }
}