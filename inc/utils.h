#pragma once

#include <jsoncpp/json/json.h>
#include <string>
#include <httplib.h>
#include <locale>

enum LOG{
    INFO, WARNING, ERROR
};

std::string do_post(std::string url, std::string endpoint, Json::Value json_message, std::map<std::string, std::string> headers = {});
std::string do_get(std::string url, std::string endpoint, std::map<std::string, std::string> headers = {});

std::string get_username(int64_t user_id, int64_t group_id);

Json::Value string_to_json(std::string str);

std::string cq_send(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
std::string cq_send(std::string end_point, Json::Value J);
void setlog(LOG type, std::string message);
int64_t get_botqq();
std::string get_local_path();

std::wstring string_to_wstring(const std::string &u);
std::string wstring_to_string(const std::wstring &u);
std::string trim(const std::string &u);
std::wstring trim(const std::wstring &u);
std::string my_replace(const std::string &s, const char old, const char ne);

int64_t get_userid(const std::wstring &s);
int64_t get_userid(const std::string &s);

int get_random(int maxi = 65536);