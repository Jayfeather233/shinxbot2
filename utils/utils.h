#pragma once

#include <curl/curl.h>
#include <filesystem>
#include <jsoncpp/json/json.h>
#include <locale>
#include <map>
#include <set>
#include <string>

#define cimg_display 0
#define cimg_use_png 1
#define cimg_use_jpeg 1
#include "CImg.h"
using namespace cimg_library;

enum LOG { INFO, WARNING, ERROR };

/**
 * message_type: "group" or "private"
 * user_id & group_id
 * message_id
 */
struct msg_meta {
    std::string message_type;
    int64_t user_id;
    int64_t group_id;
    int64_t message_id;
};

/**
 * do a http post with a json body.
 * basic headers will automatically included,
 * you can add your own headers through map<>headers
 */
std::string do_post(const std::string &httpaddr,
                    const Json::Value &json_message,
                    const std::map<std::string, std::string> &headers = {},
                    const bool proxy_flg = false);

/**
 * do a http get.
 * basic headers will automatically included,
 * you can add your own headers through map<>headers
 */
std::string do_get(const std::string &httpaddr,
                   const std::map<std::string, std::string> &headers = {},
                   const bool proxy_flg = false);
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
Json::Value string_to_json(const std::string &str);

/**
 * Determine if an element is in the Json Array.
*/
template<typename T>
bool find_in_array(const Json::Value &Ja, const T &data){
    for(Json::Value J : Ja){
        if(data == J.as<T>()){
            return true;
        }
    }
    return false;
}

/**
 * Check if someone is the operator of *bot*
 */
bool is_op(const int64_t &a);

/**
 * send a message to go-cqhttp
 * message_type can be "group" or "private"
 */
std::string cq_send(const std::string &message, const msg_meta &conf);
/**
 * send a message to go-cqhttp
 * end_point can be "send_msg", "set_friend_add_request"... that supported by
 * go-cq J is the value
 */
std::string cq_send(const std::string &end_point, const Json::Value &J);
/**
 * get some info from go-cqhttp
 * end_point can be "get_login_info"... that supported by go-cq
 */
std::string cq_get(const std::string &end_point);
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
void addRandomNoise(const std::string &filePath);

/**
 * download a image from a http address.
 * save it into "filePath/fileName"
 */
void download(const std::string &httpAddr, const std::string &filePath,
              const std::string &fileName, const bool proxy = false);

/**
 * read string from file
 */
std::string readfile(const std::string &file_path,
                     const std::string &default_content = "");

/**
 * write string to file
 */
void writefile(const std::string file_path, const std::string &content,
               bool is_append = false);

/**
 * open a file
 */
std::fstream openfile(const std::string file_path,
                      const std::ios_base::openmode mode);

/**
 * Convert a json array to a map
 */
template <typename des_type, typename data_type>
void parse_json_to_map(const Json::Value &J, std::map<des_type, data_type> &mp,
                       data_type data = true)
{
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        mp[J[i].as<des_type>()] = data;
    }
}

/**
 * Convert a map to json array
 */
template <typename des_type, typename data_type>
Json::Value parse_map_to_json(const std::map<des_type, data_type> &mp)
{
    Json::Value Ja;
    for (auto u : mp) {
        if (u.second) {
            Ja.append(u.first);
        }
    }
    return Ja;
}

/**
 * Convert a json array to a set
 */
template <typename des_type>
void parse_json_to_set(const Json::Value &J, std::set<des_type> &mp)
{
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        mp.insert(J[i].as<des_type>());
    }
}

/**
 * Convert a set to json array
 */
template <typename des_type>
Json::Value parse_set_to_json(const std::set<des_type> &mp)
{
    Json::Value Ja;
    for (auto u : mp) {
        Ja.append(u);
    }
    return Ja;
}

/**
 * Check if an element is in Json Array
*/
template <typename T>
bool json_array_contain(const Json::Value &J, const T &data){
    for(Json::Value u : J){
        try{
            if(u.as<T>() == data){
                return true;
            }
        } catch (...){ // Json::LogicError
            continue;
        }
    }
    return false;
}

/**
 * upload a file to group/folder.
 * file: reletive path
 */
void upload_file(const std::filesystem::path &file, const int64_t &group_id,
                 const std::string &path);

/**
 * Get the folder id in a group
 */
std::string get_folder_id(const int64_t &group_id, const std::string &path);

/**
 * Determine if a folder in group exists
 */
bool is_folder_exist(const int64_t &group_id, const std::string &path);

/**
 * Determine if someone is operator
 */
bool is_group_op(const int64_t &group_id, const int64_t &user_id);
