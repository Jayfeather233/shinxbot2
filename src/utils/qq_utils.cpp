#include "utils.h"

#include <filesystem>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <map>

std::string get_stranger_name(const bot *p, userid_t user_id) {
    Json::Value input;
    input["user_id"] = user_id;
    std::string res = p->cq_send("get_stranger_info", input);
    input.clear();
    input = string_to_json(res);
    if (input["data"].isNull())
        return "";
    std::string name = input["data"]["nickname"].asString();
    return name;
}

std::string get_group_member_name(const bot *p, userid_t user_id,
                                  groupid_t group_id) {
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
        name = (!input["data"]["card"].isNull() &&
                input["data"]["card"].asString().size() != 0)
                   ? input["data"]["card"].asString()
                   : input["data"]["nickname"].asString();
    return name;
}

std::string get_username(const bot *p, userid_t user_id, groupid_t group_id) {
    if (group_id == 0) {
        return get_stranger_name(p, user_id);
    } else {
        return get_group_member_name(p, user_id, group_id);
    }
}

userid_t extract_qq_from_at_segment(const std::string &seg) {
    size_t qq_pos = seg.find("qq=");
    if (qq_pos == std::string::npos) {
        return 0;
    }
    qq_pos += 3;
    size_t qq_end = seg.find_first_of(",]", qq_pos);
    if (qq_end == std::string::npos || qq_end <= qq_pos) {
        return 0;
    }
    return my_string2uint64(seg.substr(qq_pos, qq_end - qq_pos));
}

std::string display_name_in_group(const msg_meta &conf, userid_t user_id) {
    std::string name = get_username(conf.p, user_id, conf.group_id);
    if (!trim(name).empty()) {
        return name;
    }
    return "用户" + std::to_string(user_id);
}

bool is_folder_exist(const bot *p, const groupid_t &group_id,
                     const std::string &path) {
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

std::string get_folder_id(const bot *p, const groupid_t &group_id,
                          const std::string &path) {
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
void upload_file(bot *p, const fs::path &file, const groupid_t &group_id,
                 const std::string &path) {
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
        J["file"] = (fs::current_path() / file).lexically_normal().string();
        J["name"] = file.filename().string();
        J["folder"] = id;
        J = string_to_json(p->cq_send("upload_group_file", J));
        if (J.isMember("msg")) {
            p->cq_send(Json::FastWriter().write(J), msg_meta("group", 0, group_id, 0));
        }
    } catch (...) {
        p->setlog(LOG::WARNING, "File upload failed.");
        p->cq_send("File upload failed.", msg_meta("group", 0, group_id, 0));
    }
}

bool is_group_op(const bot *p, const groupid_t &group_id,
                 const userid_t &user_id) {
    if (group_id == 0)
        return false;
    Json::Value J;
    J["group_id"] = group_id;
    J["user_id"] = user_id;
    J = string_to_json(p->cq_send("get_group_member_info", J));
    if (J["data"].isNull())
        return false;
    return J["data"]["role"].asString() != "member";
}

bool is_group_member(const bot *p, const groupid_t &group_id,
                     const userid_t &user_id) {
    Json::Value J;
    J["group_id"] = group_id;
    J = string_to_json(p->cq_send("get_group_member_list", J));
    J = J["data"];
    for (auto j : J) {
        if (j["user_id"].asUInt64() == user_id) {
            return true;
        }
    }
    return false;
}

bool is_friend(const bot *p, const userid_t &user_id) {
    Json::Value J = string_to_json(p->cq_get("get_friend_list"))["data"];
    Json::ArrayIndex sz = J.size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        if (J[i]["user_id"].asUInt64() == user_id) {
            return true;
        }
    }
    return false;
}

void send_file_private(const bot *p, const userid_t user_id,
                       const fs::path &path) {
    Json::Value J;
    J["user_id"] = user_id;
    J["file"] = path.string();
    J["name"] = path.has_filename() ? path.filename().string() : "unnamed";
    p->cq_send("upload_private_file", J);
}

/**
 * This function substitutes the CQ:image segment with a local file path, which can be used for uploading files.
 * CQ format: [CQ:image,file=123.jpg,file_size=114514,sub_type=0,summary=,url=https://example.com/image.jpg]
 */
std::string substitute_image_segment(bot *p, std::string segment, const fs::path &save_dir, const std::string &cq_image_segment_template) {
    size_t cq_pos = segment.find("[CQ:image");
    while (cq_pos != std::string::npos) {
        size_t end_pos = segment.find("]", cq_pos);
        if (end_pos == std::string::npos) {
            break; // Invalid segment, no closing bracket
        }
        std::string img_segment = segment.substr(cq_pos + 10, end_pos - cq_pos - 10); // Extract the content inside [CQ:image...]
        img_segment = cq_decode(img_segment);
        std::istringstream ss(img_segment);
        std::string token;
        std::map<std::string, std::string> params;
        while (std::getline(ss, token, ',')) {
            size_t eq_pos = token.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = token.substr(0, eq_pos);
                std::string value = token.substr(eq_pos + 1);
                params[key] = value;
            }
        }
        if (params.count("file") > 0 && params.count("url") > 0) {
            std::string file_name = params["file"];
            std::string url = params["url"];
            try {
                download(url, save_dir, file_name);
                fs::path local_path = fs::absolute(save_dir / file_name);
                std::string cq_image_segment = fmt::format(cq_image_segment_template, cq_encode(local_path.string()));
                segment.replace(cq_pos, end_pos - cq_pos + 1, cq_image_segment);
            } catch (const std::exception &e) {
                p->setlog(LOG::ERROR, "Failed to download image: " + std::string(e.what()));
            }
        } else {
            p->setlog(LOG::WARNING, "Invalid CQ:image segment, missing file or url parameter.");
        }
        cq_pos = segment.find("[CQ:image", cq_pos + 1);
    }
    return segment;
}