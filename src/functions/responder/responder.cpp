#include "responder.h"
#include "utils.h"

// 固定消息回复功能
static std::string HELP =
    "固定消息回复功能。\n"
    "格式：reply.add [trigger] [response]（仅限群聊）\n"
    "格式：reply.add [group_id] [trigger] [response]（仅限私聊）\n"
    "trigger为触发消息，response为回复消息，支持{{username}}"
    "变量。（response可以在第二条消息中发出）\n"
    "reply.del [trigger]\n"
    "reply.del [group_id] [trigger]\n"
    "只能由群管理员操作。";
void Responder::process(std::string message, const msg_meta &conf)
{
    if (message == "reply.help") {
        conf.p->cq_send(HELP, conf);
        return;
    }
    auto it = is_adding.find(conf.user_id);
    if (it != is_adding.end()) {
        if (conf.message_type == "private" ||
            std::get<0>(it->second) == conf.group_id) {
            std::string trigger = std::get<1>(it->second);
            replies[std::get<0>(it->second)][trigger] =
                get_reply_message(message, conf);
            save();
            conf.p->cq_send("添加成功", conf);
        }
        is_adding.erase(it);
        return;
    }

    if (message.find("reply.add ") == 0) {
        size_t group_id = conf.group_id;
        message = trim(message.substr(10));
        if (conf.message_type != "group") {
            try {
                size_t space_pos = message.find(' ');
                group_id = std::stoull(message.substr(0, space_pos));
                message = trim(message.substr(space_pos + 1));
            }
            catch (...) {
                conf.p->cq_send(
                    "参数错误。请使用 reply.add [group_id] [trigger] "
                    "[response] 格式（response可以在第二条消息中发出）",
                    conf);
                return;
            }
        }
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, group_id, conf.user_id)) {
            return;
        }
        size_t space_pos = message.find(' ');
        if (message.empty()) {
            conf.p->cq_send("请使用 reply.add [trigger] [response] "
                            "格式（response可以在第二条消息中发出）",
                            conf);
            return;
        }
        if (space_pos == std::string::npos) {
            space_pos = message.find('[');
        }
        if (space_pos == std::string::npos) {
            conf.p->cq_send("请发送消息", conf);
            is_adding[conf.user_id] = std::make_tuple(group_id, message);
            return;
        }
        std::string trigger = trim(message.substr(0, space_pos));
        std::string response = trim(message.substr(space_pos + 1));
        replies[group_id][trigger] = get_reply_message(response, conf);
        save();
        conf.p->cq_send("添加成功", conf);
        return;
    }
    else if (message.find("reply.del ") == 0) {
        size_t group_id = conf.group_id;
        message = trim(message.substr(10));
        if (conf.message_type != "group") {
            try {
                size_t space_pos = message.find(' ');
                group_id = std::stoull(message.substr(0, space_pos));
                message = trim(message.substr(space_pos + 1));
            }
            catch (...) {
                conf.p->cq_send(
                    "参数错误。请使用 reply.add [group_id] [trigger] "
                    "[response] 格式（response可以在第二条消息中发出）",
                    conf);
                return;
            }
        }
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, group_id, conf.user_id)) {
            return;
        }
        std::string trigger = trim(message);
        if (replies[group_id].erase(trigger)) {
            save();
            conf.p->cq_send("删除成功", conf);
        }
        else {
            conf.p->cq_send("没有找到对应的触发消息", conf);
        }
        return;
    }
    else if (message.find("reply.trigger ") == 0) {
        size_t group_id = conf.group_id;
        message = trim(message.substr(14));
        if (conf.message_type != "group") {
            try {
                size_t space_pos = message.find(' ');
                group_id = std::stoull(message.substr(0, space_pos));
                message = trim(message.substr(space_pos + 1));
            }
            catch (...) {
                conf.p->cq_send(
                    "参数错误。请使用 reply.trigger [group_id] [0/1] 格式",
                    conf);
                return;
            }
        }
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, group_id, conf.user_id)) {
            conf.p->cq_send("只有群管理员可以设置触发人", conf);
            return;
        }
        std::string trigger = trim(message);
        try {
            trigger_by[group_id] = std::stoi(trigger) == 1;
            save();
            conf.p->cq_send(fmt::format("触发人设置为{}",
                                        trigger_by[group_id] ? "管理" : "全员"),
                            conf);
        }
        catch (...) {
            conf.p->cq_send("参数错误。0: 全员；1：管理", conf);
            return;
        }
        return;
    }
    else if (message.find("reply.list") == 0) {
        size_t group_id = conf.group_id;
        if (conf.message_type != "group") {
            try {
                group_id = std::stoull(trim(message.substr(10)));
            }
            catch (...) {
                conf.p->cq_send("参数错误。请使用 reply.list [group_id] 格式",
                                conf);
                return;
            }
        }
        if (!conf.p->is_op(conf.user_id) &&
            !is_group_op(conf.p, group_id, conf.user_id)) {
            return;
        }
        std::ostringstream oss;
        for (const auto &pair : replies[group_id]) {
            oss << fmt::format("{}\n", pair.first);
        }
        conf.p->cq_send(oss.str(), conf);
        return;
    }
    else {
        groupid_t groupid = conf.group_id;
        if (conf.message_type == "private") {
            try {
                message = trim(message);
                size_t space_pos = message.find(' ');
                groupid = my_string2uint64(message.substr(space_pos + 1));
                message = message.substr(0, space_pos);
            }
            catch (...) {
                return;
            }
        }
        else if (conf.message_type != "group") {
            return;
        }
        else if (trigger_by[conf.group_id] && !conf.p->is_op(conf.user_id) &&
                 !is_group_op(conf.p, conf.group_id, conf.user_id)) {
            return;
        }
        if (groupid == 0) {
            return;
        }
        auto it = replies[groupid].find(trim(message));
        if (it != replies[groupid].end()) {
            std::string response = it->second;
            size_t pos = 0;
            while ((pos = response.find("{{username}}", pos)) !=
                   std::string::npos) {
                response.replace(pos, 12,
                                 get_username(conf.p, conf.user_id, groupid));
            }
            if (response.find("[fwd]") == 0) {
                if (conf.message_type == "private") {
                    Json::Value J;
                    J["message"] =
                        string_to_json(response.substr(5))["messages"];
                    J["user_id"] = conf.user_id;
                    conf.p->cq_send("send_private_forward_msg", J);
                }
                else {
                    Json::Value J;
                    J["message"] =
                        string_to_json(response.substr(5))["messages"];
                    J["group_id"] = conf.group_id;
                    conf.p->cq_send("send_group_forward_msg", J);
                }
            }
            else {
                conf.p->cq_send(response, conf);
            }
        }
        return;
    }
}

void Responder::save()
{
    Json::Value J;
    for (const auto &group_pair : replies) {
        Json::Value group_val;
        for (const auto &reply_pair : group_pair.second) {
            group_val[reply_pair.first] = reply_pair.second;
        }
        J[std::to_string(group_pair.first)] = group_val;
    }
    if (!trigger_by.empty()) {
        for (const auto &pair : trigger_by) {
            J["trigger_by"][std::to_string(pair.first)] = pair.second ? 1 : 0;
        }
    }
    writefile(bot_config_path(nullptr, "features/responder/responder.json"),
              J.toStyledString());
}

Responder::Responder()
{
    Json::Value J;
    try {
        J = string_to_json(readfile(
            bot_config_path(nullptr, "features/responder/responder.json"),
            "{}"));
    }
    catch (...) {
        return;
    }
    for (const auto &group_member : J.getMemberNames()) {
        try {
            groupid_t group_id = std::stoull(group_member);
            const Json::Value &group_val = J[group_member];
            for (const auto &reply_member : group_val.getMemberNames()) {
                replies[group_id][reply_member] =
                    group_val[reply_member].asString();
            }
        }
        catch (...) {
            continue;
        }
    }
    if (J.isMember("trigger_by")) {
        for (const auto &member : J["trigger_by"].getMemberNames()) {
            groupid_t group_id = std::stoull(member);
            trigger_by[group_id] = J["trigger_by"][member].asInt() == 1;
        }
    }
}

bool Responder::check(std::string message, const msg_meta &conf)
{
    return conf.message_type == "group" || conf.message_type == "private";
}
std::string Responder::help() { return "固定消息回复功能：reply.help"; }

std::string substitute_image(std::string res)
{
    const std::string reply_dir = bot_resource_path("reply");
    fs::create_directories(reply_dir);
    size_t pos = 0, pos2, pos_file, file_end, pos_url, url_end;
    while ((pos = res.find("[CQ:image", pos)) != std::string::npos) {
        pos2 = res.find("]", pos);
        if (pos2 == std::string::npos)
            break;
        // download image into reply resource directory and replace with
        // [CQ:image,file=path]
        // [CQ:image,file=6AA2D5B11C7E295E597F3B2FD4B8DA6A.png,file_size=25914,sub_type=0,summary=,url=https://multimedia.nt.qq.com.cn/download?appid=1407&amp;fileid=EhQRn4b5aR_1IgECnZRSkB1eBRH8pxi6ygEg_woogeGpycCBkwMyBHByb2RQgL2jAVoQahNtvJKvteI2QkTRO5r_bHoCFrWCAQJuag&amp;rkey=CAESMCWmpmpkAP-qo8ncknjnBE0aQyAcvo5UCpcXGzO1taNxCiwH8pvTrjQqQxj3nyYZgg]
        pos_file = res.find("file=", pos);
        pos_url = res.find("url=", pos);
        if (pos_file == std::string::npos || pos_url == std::string::npos ||
            pos_file > pos2 || pos_url > pos2) {
            pos = pos2 + 1;
            continue;
        }
        file_end = res.find(",", pos_file);
        if (file_end == std::string::npos || file_end > pos2) {
            file_end = pos2;
        }
        std::string file = res.substr(pos_file + 5, file_end - pos_file - 5);
        url_end = res.find(",", pos_url);
        if (url_end == std::string::npos || url_end > pos2) {
            url_end = pos2;
        }
        std::string url = res.substr(pos_url + 4, url_end - pos_url - 4);
        std::string local_path = reply_dir + "/" + file;
        try {
            download(cq_decode(url), reply_dir + "/", file);
            res.replace(pos, pos2 - pos + 1,
                        "[CQ:image,file=file://" +
                            fs::absolute(fs::path(local_path)).string() +
                            ",id=40000]");
        }
        catch (...) {
            pos = pos2 + 1;
            continue;
        }
    }
    return res;
}
Json::Value substitue_image(const Json::Value &J)
{
    const std::string reply_dir = bot_resource_path("reply");
    fs::create_directories(reply_dir);
    Json::Value res = J;
    for (Json::Value &item : res) {
        if (item["type"].asString() == "image") {
            download(item["data"]["url"].asString(), reply_dir + "/",
                     item["data"]["file"].asString());
            Json::Value parsed;
            parsed["file"] =
                "file://" + fs::absolute(fs::path(reply_dir) /
                                         item["data"]["file"].asString())
                                .string();
            parsed["id"] = "40000";
            item["data"] = parsed;
        }
    }
    return res;
}
Json::Value substitute_Json(const Json::Value &J)
{
    Json::Value res;
    for (const Json::Value &item : J) {
        Json::Value parsed;
        parsed["type"] = "node";
        parsed["data"]["user_id"] = item["user_id"];
        parsed["data"]["nickname"] = item["sender"]["nickname"];
        parsed["data"]["content"] =
            messageArr_to_string(substitue_image(item["message"]));
        parsed["data"]["time"] = item["time"];
        res.append(parsed);
    }
    return res;
}

std::string Responder::get_reply_message(const std::string &message,
                                         const msg_meta &conf)
{
    std::string res;
    if (message.find("[CQ:forward") != std::string::npos) {
        Json::Value J;
        J["message_id"] = conf.message_id;
        J = string_to_json(conf.p->cq_send("get_forward_msg", J));
        if (J["retcode"].asInt() == 0) {
            J = J["data"];
            J["messages"] = substitute_Json(J["messages"]);
            return "[fwd]" + J.toStyledString();
        }
        else {
            conf.p->setlog(LOG::ERROR, "Failed to get forward message: " +
                                           J.toStyledString());
            conf.p->cq_send("获取转发消息失败", conf);
            return "";
        }
    }
    else {
        res = message;
        return substitute_image(res);
    }
}

void Responder::set_backup_files(archivist *p, const std::string &name)
{
    p->add_path(name, bot_resource_path("reply") + "/", "resource/reply/");
}

DECLARE_FACTORY_FUNCTIONS(Responder)