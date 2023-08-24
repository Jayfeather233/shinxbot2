#include "utils.h"

#include <filesystem>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <map>

std::map<int64_t, std::string> stranger_name;

std::string get_stranger_name(int64_t user_id)
{
    auto it = stranger_name.find(user_id);
    if (it == stranger_name.end()) {
        Json::Value input;
        input["user_id"] = user_id;
        std::string res = cq_send("get_stranger_info", input);
        input.clear();
        input = string_to_json(res);
        std::string name = input["data"]["nickname"].asString();
        stranger_name[user_id] = name;
        return name;
    }
    else {
        return it->second;
    }
}

std::map<int64_t, std::map<int64_t, std::string>> group_member_name;

std::string get_group_member_name(int64_t user_id, int64_t group_id)
{
    auto it = group_member_name.find(group_id);
    std::map<int64_t, std::string>::iterator it2;
    if (it == group_member_name.end()) {
        goto no_cache;
    }
    it2 = it->second.find(user_id);
    if (it2 == it->second.end()) {
        goto no_cache;
    }
    return it2->second;
no_cache:
    Json::Value input;
    input["user_id"] = user_id;
    input["group_id"] = group_id;
    std::string res = cq_send("get_group_member_info", input);
    std::string name;
    input.clear();
    input = string_to_json(res);
    if (input["data"].isNull())
        name = get_stranger_name(user_id);
    else
        name = input["data"]["card"].asString().size() != 0
                   ? input["data"]["card"].asString()
                   : input["data"]["nickname"].asString();
    group_member_name[group_id][user_id] = name;
    return name;
}

std::string get_username(int64_t user_id, int64_t group_id)
{
    if (group_id == -1) {
        return get_stranger_name(user_id);
    }
    else {
        return get_group_member_name(user_id, group_id);
    }
}

bool is_folder_exist(const int64_t &group_id, const std::string &path)
{
    Json::Value J;
    J["group_id"] = group_id;
    J = string_to_json(cq_send("get_group_root_files", J))["data"]["folders"];
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        if (J[i]["folder_name"].asString() == path) {
            return true;
        }
    }
    return false;
}

std::string get_folder_id(const int64_t &group_id, const std::string &path)
{
    Json::Value J;
    J["group_id"] = group_id;
    J = string_to_json(cq_send("get_group_root_files", J))["data"]["folders"];
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        if (J[i]["folder_name"].asString() == path) {
            return J[i]["folder_id"].asString();
        }
    }
    return "/";
}

/**
 * file: reletive path
 */
void upload_file(const std::filesystem::path &file, const int64_t &group_id,
                 const std::string &path)
{
    try {
        if (!is_folder_exist(group_id, path) &&
            is_group_op(group_id, get_botqq())) {
            Json::Value J;
            J["group_id"] = group_id;
            J["name"] = path;
            J = string_to_json(cq_send("create_group_file_folder", J));
        } // Need group op to do
        std::string id = get_folder_id(group_id, path);
        Json::Value J;
        J["group_id"] = group_id;
        J["file"] = (std::filesystem::current_path() / file)
                        .lexically_normal()
                        .string();
        J["name"] = file.filename().string();
        J["folder"] = id;
        // cq_send(J.toStyledString(), "group", -1, group_id);
        J = string_to_json(cq_send("upload_group_file", J));
        if (J.isMember("msg")) {
            cq_send(J.toStyledString(), (msg_meta){"group", -1, group_id, 0});
        }
    }
    catch (...) {
        setlog(LOG::WARNING, "File upload failed.");
        cq_send("File upload failed.", (msg_meta){"group", -1, group_id, 0});
    }
}

bool is_group_op(const int64_t &group_id, const int64_t &user_id)
{
    if (group_id == -1)
        return false;
    Json::Value J;
    J["group_id"] = group_id;
    J["user_id"] = user_id;
    J = string_to_json(cq_send("get_group_member_info", J))["data"];
    return J["role"].asString() != "member";
}
