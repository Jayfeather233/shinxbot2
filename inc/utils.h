#pragma once

#include <jsoncpp/json/json.h>
#include <string>
#include <locale>

enum LOG{
    INFO, WARNING, EORROR
};

std::string do_post(const char* url, Json::Value json_message);
std::string do_get(const char* url);

void username_init();
std::string get_username(long user_id, long group_id);

Json::Value string_to_json(std::string str);

std::string cq_send(std::string message, std::string message_type, long user_id, long group_id);
std::string cq_send(std::string end_point, Json::Value J);
void setlog(LOG type, std::string message);
long get_botqq();

std::wstring string_to_wstring(std::string u);
std::string wstring_to_string(std::wstring u);
std::string trim(std::string u);
std::wstring trim(std::wstring u);

long get_userid(std::wstring s);
long get_userid(std::string s);