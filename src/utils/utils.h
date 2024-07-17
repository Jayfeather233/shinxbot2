#pragma once

#include "bot.h"
#include "meta_func/backup.h"
#include "meta_func/timer.h"
#include <curl/curl.h>
#include <filesystem>
#include <jsoncpp/json/json.h>
#include <locale>
#include <map>
#include <set>
#include <string>

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
 * get user's name (group name if group_id != -1)
 */
std::string get_username(const bot *p, uint64_t user_id, uint64_t group_id);

/**
 * convert a string into json
 */
Json::Value string_to_json(const std::string &str);

/**
 * Determine if an element is in the Json Array.
 */
template <typename T> bool find_in_array(const Json::Value &Ja, const T &data)
{
    for (Json::Value J : Ja) {
        if (data == J.as<T>()) {
            return true;
        }
    }
    return false;
}

/**
 * Check if someone is the operator of *bot*
 */
bool is_op(const bot *p, const uint64_t a);

/**
 * send a message to Onebot11
 * message_type can be "group" or "private"
 */
std::string cq_send(const bot *p, const std::string &message,
                    const msg_meta &conf);
/**
 * send a message to Onebot11
 * end_point can be "send_msg", "set_friend_add_request"... that supported by
 * Onebot11 J is the value
 */
std::string cq_send(const bot *p, const std::string &end_point,
                    const Json::Value &J);
/**
 * get some info from Onebot11
 * end_point can be "get_login_info"... that supported by Onebot11
 */
std::string cq_get(const bot *p, const std::string &end_point);
/**
 * output the message to file and stdout.
 */
void setlog(bot *p, LOG type, std::string message);
void set_global_log(LOG type, std::string message);
uint64_t get_botqq(const bot *p);
/**
 * Get the path where the bot is (in absolute path)
 * like: /usr/home/name/bot/
 */
std::string get_local_path();
void input_process(bot *p, std::string *input);

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
int64_t my_string2int64(const std::wstring &s);
/**
 * convert all numbers in a string into int64_t
 */
int64_t my_string2int64(const std::string &s);
/**
 * convert all numbers in a string into uint64_t
 */
uint64_t my_string2uint64(const std::wstring &s);
/**
 * convert all numbers in a string into uint64_t
 */
uint64_t my_string2uint64(const std::string &s);

/**
 * get a random number [0, maxi)
 */
int get_random(int maxi = 65536);

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
void parse_json_to_set(const Json::Value &J, std::set<uint64_t> &mp);
void parse_json_to_set(const Json::Value &J, std::set<std::string> &mp);

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
 * Check if an element is in Json Array. True then return the inedx, false
 * return the Array length
 */
template <typename T>
Json::ArrayIndex json_array_find(const Json::Value &J, const T &data)
{
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        try {
            if (J[i].as<T>() == data) {
                return i;
            }
        }
        catch (...) { // Json::LogicError
            continue;
        }
    }
    return sz;
}

/**
 * upload a file to group/folder.
 * file: reletive path
 */
void upload_file(bot *p, const std::filesystem::path &file,
                 const uint64_t &group_id, const std::string &path);

/**
 * Get the folder id in a group
 */
std::string get_folder_id(const bot *p, const uint64_t &group_id,
                          const std::string &path);

/**
 * Determine if a folder in group exists
 */
bool is_folder_exist(const bot *p, const uint64_t &group_id,
                     const std::string &path);

/**
 * Determine if someone is operator
 */
bool is_group_op(const bot *p, const uint64_t &group_id,
                 const uint64_t &user_id);

inline bool is_digit(const char &s) { return '0' <= s && s <= '9'; }
inline bool is_word(const char &s)
{
    return ('a' <= s && s <= 'z') || ('A' <= s && s <= 'Z');
}

inline bool is_word(const wchar_t &s)
{
    return (L'a' <= s && s <= L'z') || (L'A' <= s && s <= L'Z');
}

void broadcast(std::string msg, const bot *u);

std::string to_human_string(const int64_t u);

void set_global_log(LOG type, std::string message);

bool is_friend(const bot *p, const uint64_t &user_id);

std::string cq_encode(const std::string &input);
std::string cq_decode(const std::string &input);
std::wstring cq_encode(const std::wstring &input);
std::wstring cq_decode(const std::wstring &input);
std::string messageArr_to_string(const Json::Value &J);
Json::Value string_to_messageArr(const std::string &s);

std::string get_stranger_name(const bot *p, uint64_t user_id);

void send_file_private(const bot *p, const uint64_t user_id,
                       const std::filesystem::path &path);