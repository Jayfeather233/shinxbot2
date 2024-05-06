#include "utils.h"

#include <filesystem>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <map>

static std::map<uint64_t, std::map<uint64_t, std::string>> stranger_name;

std::string get_stranger_name(const bot *p, uint64_t user_id)
{
    auto it = stranger_name.find(p->get_botqq());
    std::map<uint64_t, std::string>::iterator it2;
    if (it == stranger_name.end()) {
        goto no_cache2;
    }
    it2 = it->second.find(user_id);
    if (it2 == it->second.end()) {
        goto no_cache2;
    }
no_cache2:
    Json::Value input;
    input["user_id"] = user_id;
    std::string res = p->cq_send("get_stranger_info", input);
    input.clear();
    input = string_to_json(res);
    std::string name = input["data"]["nickname"].asString();
    stranger_name[p->get_botqq()][user_id] = name;
    return name;
}

static std::map<uint64_t, std::map<uint64_t, std::map<uint64_t, std::string>>>
    group_member_name;

std::string get_group_member_name(const bot *p, int64_t user_id,
                                  int64_t group_id)
{
    auto it1 = group_member_name.find(p->get_botqq());
    std::map<uint64_t, std::map<uint64_t, std::string>>::iterator it2;
    std::map<uint64_t, std::string>::iterator it3;
    if (it1 == group_member_name.end()) {
        goto no_cache;
    }
    it2 = it1->second.find(group_id);
    if (it2 == it1->second.end()) {
        goto no_cache;
    }
    it3 = it2->second.find(user_id);
    if (it3 == it2->second.end()) {
        goto no_cache;
    }
    return it3->second;
no_cache:
    Json::Value input;
    input["user_id"] = user_id;
    input["group_id"] = group_id;
    std::string res = p->cq_send("get_group_member_info", input);
    std::string name;
    input.clear();
    input = string_to_json(res);
    if (input["data"].isNull())
        name = get_stranger_name(p, user_id);
    else
        name = input["data"]["card"].asString().size() != 0
                   ? input["data"]["card"].asString()
                   : input["data"]["nickname"].asString();
    group_member_name[p->get_botqq()][group_id][user_id] = name;
    return name;
}

std::string get_username(const bot *p, uint64_t user_id, uint64_t group_id)
{
    if (group_id == 0) {
        return get_stranger_name(p, user_id);
    }
    else {
        return get_group_member_name(p, user_id, group_id);
    }
}

bool is_folder_exist(const bot *p, const uint64_t &group_id,
                     const std::string &path)
{
    Json::Value J;
    J["group_id"] = group_id;
    J = string_to_json(
        p->cq_send("get_group_root_files", J))["data"]["folders"];
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        if (J[i]["folder_name"].asString() == path) {
            return true;
        }
    }
    return false;
}

std::string get_folder_id(const bot *p, const uint64_t &group_id,
                          const std::string &path)
{
    Json::Value J;
    J["group_id"] = group_id;
    J = string_to_json(
        p->cq_send("get_group_root_files", J))["data"]["folders"];
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
void upload_file(bot *p, const std::filesystem::path &file,
                 const uint64_t &group_id, const std::string &path)
{
    try {
        if (!is_folder_exist(p, group_id, path) &&
            is_group_op(p, group_id, p->get_botqq())) {
            Json::Value J;
            J["group_id"] = group_id;
            J["name"] = path;
            J = string_to_json(p->cq_send("create_group_file_folder", J));
        } // Need group op to do
        std::string id = get_folder_id(p, group_id, path);
        Json::Value J;
        J["group_id"] = group_id;
        J["file"] = (std::filesystem::current_path() / file)
                        .lexically_normal()
                        .string();
        J["name"] = file.filename().string();
        J["folder"] = id;
        // cq_send(J.toStyledString(), "group", -1, group_id);
        J = string_to_json(p->cq_send("upload_group_file", J));
        if (J.isMember("msg")) {
            p->cq_send(J.toStyledString(), (msg_meta){"group", 0, group_id, 0});
        }
    }
    catch (...) {
        p->setlog(LOG::WARNING, "File upload failed.");
        p->cq_send("File upload failed.", (msg_meta){"group", 0, group_id, 0});
    }
}

bool is_group_op(const bot *p, const uint64_t &group_id,
                 const uint64_t &user_id)
{
    if (group_id == 0)
        return false;
    Json::Value J;
    J["group_id"] = group_id;
    J["user_id"] = user_id;
    J = string_to_json(p->cq_send("get_group_member_info", J))["data"];
    return J["role"].asString() != "member";
}

bool is_friend(const bot *p, const uint64_t &user_id)
{
    Json::Value J = string_to_json(p->cq_get("get_friend_list"))["data"];
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        if (J[i]["user_id"].asUInt64() == user_id) {
            return true;
        }
    }
    return false;
}

void send_file_private(const bot *p, const uint64_t user_id,
                       const std::filesystem::path &path)
{
    Json::Value J;
    J["user_id"] = user_id;
    J["file"] = path.string();
    J["name"] = path.has_filename() ? path.filename().string() : "unnamed";
    p->cq_send("upload_private_file", J);
}