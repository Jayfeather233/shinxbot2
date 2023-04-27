/**
 * !! You should not read, test or modify this code
 *    Unless you are over 18 years old !!
 */

#include "621.h"
#include "processable.h"
#include "utils.h"
#include <base64.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <zip.h>

e621::e621()
{
    std::string ans = readfile("./config/621_level.json", "{}");
    Json::Value J = string_to_json(ans);

    username = J["username"].asString();
    authorkey = J["authorkey"].asString();
    parse_json_to_map(J["group"], group);
    parse_json_to_map(J["user"], user);
    Json::ArrayIndex sz = J["n_search"].size();
    for (Json::ArrayIndex i = 0; i < sz; i++) {
        n_search.insert(J["n_search"][i].asString());
    }
}

std::string e621::deal_input(const std::string &input, bool is_pool)
{
    std::string res = my_replace(input, ' ', '+');
    if (input.length() < 1) {
        res += "+pokemon+fav:jayfeather233";
    }
    if (input[0] == '+') {
        res += "+fav:jayfeather233";
    }
    bool is_id = res.find("id:") != res.npos;

    if (res.find("score:") == res.npos && res.find("favcount:") == res.npos &&
        !is_pool && !is_id) {
        res += "+score:>200+favcount:>400";
    }
    if (res.find("order:") == res.npos && !is_pool) {
        res += "+order:random";
    }
    // res += "+-animated";
    if (!is_pool && !is_id)
        for (std::string it : n_search) {
            if (res.find(it) == res.npos) {
                res += "+-" + it;
            }
        }
    return res;
}

std::string number_trans(int64_t u)
{
    if (u > 1000000)
        return std::to_string(u / 1000000) + "M";
    else if (u > 1000)
        return std::to_string(u / 1000) + "k";
    else
        return std::to_string(u);
}

void e621::process(std::string message, const msg_meta &conf)
{
    message = trim(message);
    if (message.find("621.add") == 0) {
        message = trim(message.substr(7));
        admin_set(message, conf, true);
        return;
    }
    if (message.find("621.del") == 0) {
        message = trim(message.substr(7));
        admin_set(message, conf, false);
        return;
    }

    if (conf.message_type == "group") {
        if (!group[conf.group_id]) {
            setlog(LOG::WARNING, "621 in group " +
                                     std::to_string(conf.group_id) +
                                     " but no permission");
            return;
        }
    }
    else {
        if (!user[conf.user_id]) {
            setlog(LOG::WARNING, "621 at user " + std::to_string(conf.user_id) +
                                     " but no permission");
            return;
        }
    }

    if (message == "621.default") {
        std::string res =
            "如未指定任何内容，默认加上fav:jayfeather233 pokemon\n"
            "如未指定favcount或score，默认加上favcount:>400 score:>200\n"
            "如未指定以下tags，默认不搜索";
        for (std::string it : n_search) {
            res += it + ",";
        }
        cq_send(res, conf);
        return;
    }
    if (message.find("621.autocomplete") == 0) {
        message = trim(message.substr(16));
        try {
            Json::Value Ja = string_to_json(do_get(
                "https://e621.net/tags/"
                "autocomplete.json?search[name_matches]=" +
                    message + "&expiry=7",
                {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                 {"Authorization",
                  "basic " + base64::to_base64(username + ":" + authorkey)}},
                true));
            std::string res;
            Json::ArrayIndex sz = Ja.size();
            for (Json::ArrayIndex i = 0; i < sz; i++) {
                res += Ja[i]["name"].asString() + "    " +
                       std::to_string(Ja[i]["post_count"].asInt64()) + "\n";
            }
            cq_send(res, conf);
        }
        catch (...) {
            cq_send("621 connect error. Try again.", conf);
        }
        return;
    }

    bool is_pool = (message.find("pool:") != message.npos);
    std::string input = message.substr(3);
    bool get_tag = false;
    if (input.find(".tag") == 0) {
        get_tag = true;
        input = trim(input.substr(4));
    }
    if (input.find(".input") == 0) {
        cq_send(deal_input(input.substr(6), is_pool), conf);
        return;
    }
    input = deal_input(input, is_pool);

    Json::Value J;
    int i;
    for (i = 0; i < 3; i++) {
        try {
            J = string_to_json(do_get(
                "https://e621.net/posts.json?limit=50&tags=" + input,
                {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
                 {"Authorization",
                  "basic " + base64::to_base64(username + ":" + authorkey)}},
                true));
            break;
        }
        catch (...) {
        }
    }
    if (i == 3) {
        cq_send("Unable to connect to e621", conf);
        setlog(LOG::WARNING, "621 at group " + std::to_string(conf.group_id) +
                                 " by " + std::to_string(conf.user_id) +
                                 " but unable to connect.");
        return;
    }

    Json::ArrayIndex count = J["posts"].size();
    if (count == 0) {
        cq_send("No image found.", conf);
        setlog(LOG::WARNING, "621 at group " + std::to_string(conf.group_id) +
                                 " by " + std::to_string(conf.user_id) +
                                 " but no image found.");
        return;
    }

    if (!is_pool) {
        Json::Value J2;
        J = J["posts"][0];
        int i;
        for (i = 0; i < 3; i++) {
            if (get_tag) {
                J2 = string_to_json(cq_send(
                    "[CQ:reply,id=" + std::to_string(conf.message_id) + "] " +
                        get_image_tags(J) + (i ? "\ntx原因无法发送原图" : ""),
                    conf));
            }
            else {
                J2 = string_to_json(cq_send(
                    "[CQ:reply,id=" + std::to_string(conf.message_id) + "] " +
                        get_image_info(J, count, is_pool, i, conf.group_id) +
                        (i ? "\ntx原因无法发送原图" : ""),
                    conf));
            }
            if (J2["status"].asString() != "failed") {
                break;
            }
        }
        if (i == 3) {
            cq_send("[CQ:reply,id=" + std::to_string(conf.message_id) +
                        "] cannot send image due to Tencent",
                    conf);
            setlog(LOG::WARNING,
                   "621 at group " + std::to_string(conf.group_id) + " by " +
                       std::to_string(conf.user_id) + " send failed.");
        }
        else {
            setlog(LOG::INFO, "621 at group " + std::to_string(conf.group_id) +
                                  " by " + std::to_string(conf.user_id));
        }
    }
    else {
        int64_t pool_id = J["posts"][0]["pools"][0].asInt64();

        Json::Value J3 = string_to_json(do_get(
            "https://e621.net/pools.json?search[id]=" + std::to_string(pool_id),
            {{"user-agent", "AutoSearch/1.0 (by " + username + " on e621)"},
             {"Authorization",
              "basic " + base64::to_base64(username + ":" + authorkey)}},
            true))[0];

        std::string res_message;
        res_message = "转发\n";
        res_message += std::to_string(get_botqq()) + " " +
                       J3["category"].asString() + ": " +
                       J3["name"].asString() + "\n";
        res_message += std::to_string(get_botqq()) + " 合并行\n简介：" +
                       J3["description"].asString() + "\n结束合并\n";
        res_message += std::to_string(get_botqq()) + " 共有 " +
                       std::to_string(J3["post_count"].asInt64()) + "张\n";
        J3 = J3["post_ids"];
        Json::ArrayIndex sz = J3.size();
        for (Json::ArrayIndex i = 0; i < sz; i++) {
            for (Json::ArrayIndex j = 0; j < count; j++) {
                if (J3[i].asInt64() == J["posts"][j]["id"].asInt64()) {
                    res_message += std::to_string(get_botqq()) + " 合并行\n";
                    res_message += get_image_info(J["posts"][j], count, is_pool,
                                                  1, conf.group_id);
                    res_message += "\n结束合并\n";
                }
            }
        }

        Json::Value J_send;
        J_send["post_type"] = "message";
        J_send["message"] = res_message;
        J_send["message_type"] = conf.message_type;
        J_send["message_id"] = -1;
        J_send["user_id"] = conf.user_id;
        J_send["group_id"] = conf.group_id;
        input_process(new std::string(J_send.toStyledString()));
        setlog(LOG::INFO, "621 pool at group " + std::to_string(conf.group_id) +
                              " by " + std::to_string(conf.user_id));
    }
}

void e621::admin_set(std::string message, const msg_meta &conf, bool flg)
{
    std::istringstream iss(message);
    if (!is_op(conf.user_id))
        return;
    std::string type;
    iss >> type;
    if (type == "this") {
        if (conf.message_type == "group") {
            group[conf.group_id] = flg;
        }
        else {
            user[conf.user_id] = flg;
        }
    }
    else {
        int64_t id;
        iss >> id;
        if (type == "group") {
            group[id] = flg;
        }
        else if (type == "user") {
            user[id] = flg;
        }
        else {
            cq_send("621.set [this/group/user] [id (when not 'this')]", conf);
        }
    }
    save();
    cq_send("set done.", conf);
}
void e621::save()
{
    Json::Value J;
    J["authorkey"] = authorkey;
    J["username"] = username;
    J["user"] = parse_map_to_json(user);
    J["group"] = parse_map_to_json(group);
    Json::Value J2;
    for (std::string u : n_search) {
        J2.append(u);
    }
    J["n_search"] = J2;

    writefile("./config/621_level.json", J.toStyledString());
}
std::string e621::get_image_tags(const Json::Value &J)
{
    std::string s;
    Json::ArrayIndex sz;
    Json::Value J2;
    J2 = J["tags"];
    s += "artist:";
    sz = J2["artist"].size();
    for (Json::ArrayIndex i = 0; i < sz; i++)
        s += J2["artist"][i].asString() + " ";
    s += "\n";
    s += "character:";
    sz = J2["character"].size();
    for (Json::ArrayIndex i = 0; i < sz; i++)
        s += J2["character"][i].asString() + " ";
    s += "\n";
    s += "species:";
    sz = J2["species"].size();
    for (Json::ArrayIndex i = 0; i < sz; i++)
        s += J2["species"][i].asString() + " ";
    s += "\n";
    return s;
}

std::string e621::get_image_info(const Json::Value &J, size_t count,
                                 bool poolFlag, int retry, int64_t group_id)
{
    std::string imageUrl;
    if (J.isMember("file") && retry <= 0) {
        imageUrl = J["file"]["url"].asString();
    }
    else if (J.isMember("sample") && retry <= 1) {
        imageUrl = J["sample"]["url"].asString();
    }
    else if (J.isMember("preview")) {
        imageUrl = J["preview"]["url"].asString();
    }
    else {
        throw "";
    }

    int64_t id = J["id"].asInt64();
    int64_t fav_count = J["fav_count"].asInt64();
    int64_t score = J["score"]["total"].asInt64();

    std::stringstream quest;
    if (!poolFlag && count != 50) {
        quest << "只有" << count << "个图片\n";
    }
    if (poolFlag && count == 50) {
        quest << "多于" << count << "个图片\n";
    }

    size_t extPos = 0, tmpPos;
    while ((tmpPos = imageUrl.find(".", extPos)) != std::string::npos) {
        extPos = tmpPos + 1;
    }
    std::string fileExt = imageUrl.substr(extPos);
    std::string imageLocalPath = std::to_string(id) + '.' + fileExt;

    bool is_downloaded = false;
    if (!std::ifstream("./resource/download/e621/" + imageLocalPath)) {
        for (int i = 0; i < 3; i++) {
            try {
                download(imageUrl, "./resource/download/e621", imageLocalPath,
                         true);
                is_downloaded = true;
                break;
            }
            catch (...) {
            }
        }
    }
    else {
        is_downloaded = true;
    }
    if (is_downloaded && fileExt != "gif" && fileExt != "webm" &&
        fileExt != "mp4")
        addRandomNoise("./resource/download/e621/" + imageLocalPath);

    if (is_downloaded && fileExt != "webm" && fileExt != "mp4") {
        quest << (fileExt == "gif" ? "Get gif:\n" : "")
              << "[CQ:image,file=file://" << get_local_path()
              << "/resource/download/e621/" << imageLocalPath << ",id=40000]\n";
    }
    else if (is_downloaded) {
        std::string zip_name =
            "./resource/download/e621/" + std::to_string(id) + ".zip";
        std::string file_name = "./resource/download/e621/" + imageLocalPath;

        zip_t *archive =
            zip_open(zip_name.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr);
        if (archive == NULL) {
            quest << "zip创建出错" << std::endl;
        }
        else {
            zip_source_t *source1 =
                zip_source_file(archive, file_name.c_str(), 0, -1);
            int ret = zip_file_add(archive, imageLocalPath.c_str(), source1,
                                   ZIP_FL_ENC_GUESS);
            if (ret < 0) {
                zip_source_free(source1);
            }
            zip_source_t *source2 = zip_source_buffer(archive, nullptr, 0, 0);
            ret = zip_file_add(archive, "密码就是文件名", source2,
                               ZIP_FL_ENC_GUESS);
            if (ret < 0) {
                zip_source_free(source1);
            }
            zip_stat_t st;
            zip_stat(archive, imageLocalPath.c_str(), 0, &st);
            zip_file_set_encryption(archive, st.index, ZIP_EM_AES_256,
                                    std::to_string(id).c_str());
            zip_close(archive);
            upload_file(zip_name, group_id, "e621");
        }

        int ret = system(
            ("ffmpeg -y -i " + file_name + " -vframes 1 " + file_name + ".png")
                .c_str());

        if (ret != 0) {
            quest << "获取视频封面出错" << std::endl;
        }
        else {
            quest << "[CQ:image,file=file://" << get_local_path()
                  << file_name.substr(1) << ".png,id=40000]" << std::endl;
        }

        quest << "Get video. id: " + std::to_string(id) << std::endl;
    }
    else {
        quest << "图片下载失败" << std::endl;
    }
    quest << "Fav_count: " << fav_count << "  Score: " << score << "\n";

    auto poolList = J["pools"];
    if (poolList.isArray() && poolList.size() > 0) {
        quest << "pools:";
        for (Json::ArrayIndex i = 0; i < poolList.size(); i++) {
            quest << " " << poolList[i].asInt();
        }
        quest << '\n';
    }
    quest << "id: " << id;
    return quest.str();
}

bool e621::check(std::string message, const msg_meta &conf)
{
    if ((!message.find("621")) == 0)
        return false;
    if (conf.message_type == "group") {
        auto it = group.find(conf.group_id);
        if (it == group.end() || it->second == false) {
            return false;
        }
    }
    else {
        auto it = user.find(conf.user_id);
        if (it == user.end() || it->second == false) {
            return false;
        }
    }
    return true;
}
std::string e621::help() { return ""; }
