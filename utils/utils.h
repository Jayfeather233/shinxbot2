#pragma once

#include <jsoncpp/json/json.h>
#include <string>
#include <curl/curl.h>
#include <locale>

#define cimg_display 0
#define cimg_use_png 1
#define cimg_use_jpeg 1
#include "CImg.h"
using namespace cimg_library;

enum LOG{
    INFO, WARNING, ERROR
};

/**
 * do a http post with a json body.
 * basic headers will automatically included,
 * you can add your own headers through map<>headers
*/
std::string do_post(const std::string &httpaddr, const Json::Value &json_message, const std::map<std::string, std::string> &headers = {});
/**
 * do a http get.
 * basic headers will automatically included,
 * you can add your own headers through map<>headers
*/
std::string do_get(const std::string &httpaddr, const std::map<std::string, std::string> &headers = {});
/**
 * divide a http address.
 * Example: url = https://www.abc.com/hello/world
 * return = pair <https://www.abc.com, hello/world>
*/
std::pair<std::string, std::string> divide_http_addr(const std::string &url);

/**
 * get user's name (group name if group_id != -1)
*/
std::string get_username(int64_t user_id, int64_t group_id);

/**
 * convert a string into json
*/
Json::Value string_to_json(std::string str);

/**
 * send a message to go-cqhttp
 * message_type can be "group" or "private"
*/
std::string cq_send(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
/**
 * send a message to go-cqhttp
 * end_point can be "send_msg", "set_friend_add_request"... that supported by go-cq
 * J is the value
*/
std::string cq_send(std::string end_point, Json::Value J);
/**
 * get some info from go-cqhttp
 * end_point can be "get_login_info"... that supported by go-cq
*/
std::string cq_get(std::string end_point);
/**
 * output the message to file and stdout.
*/
void setlog(LOG type, std::string message);
int64_t get_botqq();
/**
 * Get the path where the bot is (in absolute path)
 * like: /usr/home/name/bot/
*/
std::string get_local_path();
void input_process(std::string *input);

std::wstring string_to_wstring(const std::string &u);
std::string wstring_to_string(const std::wstring &u);
/**
 * delete white characters at the begin or end of a string
*/
std::string trim(const std::string &u);
/**
 * delete white characters at the begin or end of a string
*/
std::wstring trim(const std::wstring &u);
/**
 * replace all old char to ne char
*/
std::string my_replace(const std::string &s, const char old, const char ne);

/**
 * convert all numbers in a string into int64_t
*/
int64_t get_userid(const std::wstring &s);
/**
 * convert all numbers in a string into int64_t
*/
int64_t get_userid(const std::string &s);

/**
 * get a random number(smaller than 65536)
*/
int get_random(int maxi = 65536);

/**
 * add random noise to a image
*/
void addRandomNoise(const std::string& filePath);
/**
 * download a image from a http address.
 * save it into "filePath/fileName"
*/
void download(const std::string& httpAddr, const std::string& filePath, const std::string& fileName);