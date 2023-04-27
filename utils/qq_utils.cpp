#include "utils.h"

#include <filesystem>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <map>
std::string get_stranger_info(int64_t user_id)
{
    Json::Value input;
    input["user_id"] = user_id;
    std::string res = cq_send("get_stranger_info", input);
    input.clear();
    input = string_to_json(res);
    return input["data"]["nickname"].asString();
}

std::string get_group_member_info(int64_t user_id, int64_t group_id)
{
    Json::Value input;
    input["user_id"] = user_id;
    input["group_id"] = group_id;
    std::string res = cq_send("get_group_member_info", input);
    input.clear();
    input = string_to_json(res);
    if (input["data"].isNull())
        return get_stranger_info(user_id);
    else
        return input["data"]["card"].asString().size() != 0
                   ? input["data"]["card"].asString()
                   : input["data"]["nickname"].asString();
}

std::string get_username(int64_t user_id, int64_t group_id)
{
    if (group_id == -1) {
        return get_stranger_info(user_id);
    }
    else {
        return get_group_member_info(user_id, group_id);
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
        } // Cannot use?
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
            cq_send(
                J.toStyledString(), (msg_meta){"group", -1, group_id, 0});
        }
    }
    catch (...) {
        setlog(LOG::WARNING, "File upload failed.");
        cq_send(
            "File upload failed.", (msg_meta){"group", -1, group_id, 0});
    }
}

bool is_group_op(const int64_t &group_id, const int64_t &user_id)
{
    Json::Value J;
    J["group_id"] = group_id;
    J["user_id"] = user_id;
    J = string_to_json(cq_send("get_group_member_info", J))["data"];
    return J["role"].asString() != "member";
}
