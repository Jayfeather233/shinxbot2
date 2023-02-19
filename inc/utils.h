#pragma once

#include <jsoncpp/json/json.h>
#include <string>
//#include <httplib.h>
#include <curl/curl.h>
#include <locale>

#define cimg_display_type 0
#define cimg_use_png 1
#include "CImg.h"
using namespace cimg_library;

enum LOG{
    INFO, WARNING, ERROR
};

std::string do_post(const std::string &url, const std::string &endpoint, const Json::Value &json_message, const std::map<std::string, std::string> &headers = {});
std::string do_get(const std::string &url, const std::string &endpoint, const std::map<std::string, std::string> &headers = {});
std::pair<std::string, std::string> divide_http_addr(const std::string &url);

std::string get_username(int64_t user_id, int64_t group_id);

Json::Value string_to_json(std::string str);

std::string cq_send(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
std::string cq_send(std::string end_point, Json::Value J);
std::string cq_get(std::string end_point);
void setlog(LOG type, std::string message);
int64_t get_botqq();
std::string get_local_path();
void input_process(std::string *input);

std::wstring string_to_wstring(const std::string &u);
std::string wstring_to_string(const std::wstring &u);
std::string trim(const std::string &u);
std::wstring trim(const std::wstring &u);
std::string my_replace(const std::string &s, const char old, const char ne);

int64_t get_userid(const std::wstring &s);
int64_t get_userid(const std::string &s);

int get_random(int maxi = 65536);

void addRandomNoise(const std::string& filePath);
void download(const std::string& httpAddr, const std::string& filePath, const std::string& fileName);