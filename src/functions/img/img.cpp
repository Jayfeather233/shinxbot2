#include "img.h"

#include "utils.h"
#include <filesystem>
#include <iostream>
#include <mutex>

static std::string help_message = "美图 帮助 - 列出所有美图命令\n"
                                  "美图 列表 - 列出本群美图\n"
                                  "美图 加入 xxx - 加入一张图片至xxx类，不存在会创建（仅限群聊）\n"
                                  "美图 加入群 groupid xxx - 加入一张图片至groupid群的xxx类，不存在会创建（仅限私聊）\n"
                                  "xxx - 发送美图（随机或指定一个数字）（仅限群聊）\n"
                                  "xxx groupid - 发送对应群内美图（随机或指定一个数字）（仅限私聊）";

img::img()
{
    Json::Value J = string_to_json(readfile("./config/img.json", "{}"));
    for (const auto &item : J["default_images"]) {
        default_image_list.push_back(item.asString());
    }
    for (const auto &item : J["image_size"].getMemberNames()) {
        image_size[item] = J["image_size"][item].asUInt64();
        image_list.push_back(item);
    }
    for (const auto &item : J["name"].getMemberNames()) {
        name_uuid.insert(string_to_wstring(J["name"][item].asString()), item);
    }
    for (const auto &item : J["belongs"].getMemberNames()) {
        groupid_t group_id = static_cast<groupid_t>(std::stoll(item));
        for (const auto &uuid_item : J["belongs"][item]) {
            std::string uuid = uuid_item.asString();
            uuid_groupid.insert(uuid, group_id);
        }
    }
}

std::mutex img_mutex;
std::mutex save_mutex;

void img::save()
{
    std::lock_guard<std::mutex> lock_guard(save_mutex);
    Json::Value J;
    J["default_images"] = Json::Value(Json::arrayValue);
    for (const auto &it : default_image_list) {
        J["default_images"].append(it);
    }
    J["image_size"] = Json::Value(Json::objectValue);
    for (const auto &it : image_size) {
        if (it.second == 0) {
            continue;
        }
        J["image_size"][it.first] = Json::Value(Json::UInt64(it.second));
    }
    J["name"] = Json::Value(Json::objectValue);
    for (const auto &it : name_uuid.get_by_second()) {
        auto names = name_uuid.get_by_second(it);
        size_t img_size = image_size[it];
        if (!names.empty() && img_size > 0) {
            J["name"][it] = wstring_to_string(names[0]);
        }
    }
    J["belongs"] = Json::Value(Json::objectValue);
    for (const auto &it : uuid_groupid.get_by_second()) {
        Json::Value uuid_array = Json::Value(Json::arrayValue);
        for (const auto &uuid : uuid_groupid.get_by_second(it)) {
            if (image_size[uuid] == 0) {
                continue;
            }
            uuid_array.append(Json::Value(uuid));
        }
        J["belongs"][std::to_string(it)] = uuid_array;
    }
    writefile("./config/img.json", J.toStyledString());
}

void img::add_images(const std::wstring &message,
                     const std::string &name,
                     const groupid_t &groupid,
                     const msg_meta &conf)
{
    std::lock_guard<std::mutex> lock_guard(img_mutex);
    std::string uuid;
    const auto &uuids = uuid_groupid.get_by_second(groupid); // 此群内的图集
    for (const auto &u : uuids) {
        const auto &names = name_uuid.get_by_second(u);
        if (std::find(names.begin(), names.end(), string_to_wstring(name)) != names.end()) {
            uuid = u;
            break;
        }
    }
    if (uuid.empty()) {
        do {
            uuid = generate_uuid();
        } while(std::find(image_list.begin(), image_list.end(), uuid) != image_list.end());
        name_uuid.insert(string_to_wstring(name), uuid);
        uuid_groupid.insert(uuid, groupid);
        image_size[uuid] = 0;
        image_list.push_back(uuid);
    }

    size_t index = 0;
    size_t count = 0;
    while ((index = message.find(L"[CQ:image,", index)) != std::wstring::npos) {
        size_t index2 = message.find(L",url=", index);
        index2 += 5; // length of ",url="
        size_t index3 = message.find_first_of(L",]", index2);
        if (index3 == std::wstring::npos) {
            conf.p->cq_send("数据有误，请重试", conf);
            return;
        }
        std::string image = wstring_to_string(message.substr(index2, index3 - index2));
        conf.p->setlog(LOG::INFO, "img add image url found: " + image);
        fs::path dir_path = "./resource/mt/" + uuid;
        fs::create_directories(dir_path);
        download(cq_decode(image), dir_path.string(), std::to_string(image_size[uuid]));
        image_size[uuid] += 1;
        count += 1;

        index = index3 + 1;
    }
    save();
    
    if (count > 1) {
        conf.p->cq_send("已加入 " + std::to_string(count) + " 张至 " + name, conf);
    } else {
        conf.p->cq_send("已加入 " + name, conf);
    }
}

bool is_in_group(groupid_t gid, userid_t uid, const bot *p) {
    Json::Value J = Json::Value(Json::objectValue);
    J["group_id"] = gid;
    J["user_id"] = uid;
    J = string_to_json(p->cq_send("get_group_member_info", J));
    if (J["retcode"].asInt() != 0) {
        return false;
    } else {
        return true;
    }
}

bool img::process_add_images(const std::wstring &msg,
                             const std::string &name,
                             const groupid_t &groupid,
                             const msg_meta &conf) {
    std::string name2;
    size_t pos = name.find("[CQ:");
    if (pos != std::string::npos) {
        name2 = name.substr(0, pos);
    } else {
        name2 = name;
    }
    if (name2.empty()) {
        conf.p->cq_send("名称不能为空", conf);
        return true;
    }
    if (!is_in_group(groupid, conf.user_id, conf.p)) {
        conf.p->cq_send(fmt::format("用户 {} 不在群 {} 内", conf.user_id, groupid), conf);
        return true;
    }
    if (msg.find(L"[CQ:image,") == std::wstring::npos) {
        is_adding[conf.user_id] = std::make_tuple(true, groupid, string_to_wstring(name2));
        conf.p->cq_send("图来！", conf);
        return true;
    }
    
    add_images(msg, name2, conf.group_id, conf);
    conf.p->setlog(LOG::INFO, fmt::format("美图 {} add by {}", name2,
                                            std::to_string(conf.user_id)));
    return true;
}

void img::del_images(const std::wstring &name,
                     const groupid_t &groupid,
                     const std::string &index,
                     const msg_meta &conf) {
    std::lock_guard<std::mutex> lock_guard(img_mutex);
    std::string uuid;
    const auto &uuids = uuid_groupid.get_by_second(groupid); // 此群内的图集
    for (const auto &u : uuids) {
        const auto &names = name_uuid.get_by_second(u);
        if (std::find(names.begin(), names.end(), name) != names.end()) {
            uuid = u;
            break;
        }
    }
    if (uuid.empty()) {
        conf.p->cq_send("图集不存在", conf);
        return;
    }

    size_t count = 0;
    size_t index_num = 0;
    if (index == "all") {
        fs::path dir_path = "./resource/mt/" + uuid;
        if (fs::exists(dir_path)) {
            fs::remove_all(dir_path);
        }
        name_uuid.erase_by_second(uuid);
        uuid_groupid.erase_by_first(uuid);
        image_list.erase(std::remove(image_list.begin(), image_list.end(), uuid), image_list.end());
        image_size[uuid] = 0;
        save();
        conf.p->cq_send("已删除全部图片", conf);
        return;
    }
    
    try {
        index_num = std::stoi(index);
        index_num -= 1; // 用户从1开始计数
    } catch (...) {
        conf.p->cq_send("索引无效", conf);
        return;
    }
    
    fs::path dir_path = "./resource/mt/" + uuid;
    if (!fs::exists(dir_path)) {
        conf.p->cq_send("图集不存在", conf);
        return;
    }

    size_t img_size = image_size[uuid];
    if (img_size == 0) {
        conf.p->cq_send("图集为空", conf);
        return;
    }

    if (index_num >= img_size || index_num < 0) {
        conf.p->cq_send("索引超出范围", conf);
        return;
    }

    fs::remove(dir_path / std::to_string(index_num));

    for (size_t i = index_num + 1; i < img_size; ++i) {
        fs::rename(dir_path / std::to_string(i), dir_path / std::to_string(i - 1));
    }
    
    image_size[uuid] -= 1;

    if (image_size[uuid] == 0) {
        name_uuid.erase_by_second(uuid);
        uuid_groupid.erase_by_first(uuid);
        image_list.erase(std::remove(image_list.begin(), image_list.end(), uuid), image_list.end());
    }
    
    save();
    conf.p->cq_send(fmt::format("已删除图集 {}: {}", wstring_to_string(name), index), conf);
}

bool img::process_del_images(const std::wstring &name,
                        const groupid_t &groupid,
                        const std::string &index,
                        const msg_meta &conf) {
    if (name.empty()) {
        conf.p->cq_send("名称不能为空", conf);
        return true;
    }
    if (!is_in_group(groupid, conf.user_id, conf.p)) {
        conf.p->cq_send(fmt::format("用户 {} 不在群 {} 内", conf.user_id, groupid), conf);
        return true;
    }
    if (!is_op(conf.p, conf.user_id) && !is_group_op(conf.p, groupid, conf.user_id)) {
        conf.p->cq_send("只有管理员才能删除美图", conf);
        return true;
    }
    
    del_images(name, conf.group_id, index, conf);
    conf.p->setlog(LOG::INFO, fmt::format("美图 {} del by {}", wstring_to_string(name),
                                            std::to_string(conf.user_id)));
    return true;
}

/**
 * Return true if processed a command, and skip normal processing.
 */
bool img::process_command(std::string message, const msg_meta &conf)
{
    std::wstring wmessage = string_to_wstring(message);
    if (wmessage == L"美图 帮助") {
        conf.p->cq_send(help_message, conf);
        return true;
    }
    if (wmessage == L"美图 列表" && conf.message_type == "group") {
        std::vector<std::string> group_images_uuid =
            uuid_groupid.get_by_second(conf.group_id);
        std::ostringstream oss;
        oss << "转发" << std::endl;
        oss << std::to_string(conf.p->get_botqq()) << " 合并行" << std::endl;
        size_t cnt = 0;
        for (const auto &uuid : group_images_uuid) {
            const auto &names = name_uuid.get_by_second(uuid);
            size_t size = image_size[uuid];
            for (const auto &name : names) {
                oss << wstring_to_string(name) << " （" << size << "）"
                    << std::endl;
                cnt++;
                if (cnt % 30 == 0) {
                    oss << "结束合并" << std::endl;
                    oss << std::to_string(conf.p->get_botqq()) << " 合并行" << std::endl;
                }
            }
        }
        oss << "结束合并" << std::endl;
        Json::Value J_send;
        J_send["post_type"] = "message";
        J_send["message"] = oss.str();
        J_send["message_type"] = conf.message_type;
        J_send["message_id"] = conf.message_id;
        J_send["user_id"] = conf.user_id;
        J_send["group_id"] = conf.group_id;
        conf.p->input_process(new std::string(J_send.toStyledString()));
        return true;
    }
    if (wmessage.find(L"美图 加入 ") == 0 && conf.message_type == "group") {
        std::wstring msg = trim(wmessage.substr(5));
        std::istringstream iss(wstring_to_string(msg));
        std::string name;
        iss >> name;
        return process_add_images(msg, name, conf.group_id, conf);
    }
    if (wmessage.find(L"美图 加入群 ") == 0 && conf.message_type == "private") {
        std::wstring msg = trim(wmessage.substr(6));
        std::istringstream iss(wstring_to_string(msg));
        groupid_t gid;
        std::string name;
        iss >> gid >> name;
        return process_add_images(msg, name, gid, conf);
    }
    if (wmessage.find(L"美图 删除 ") == 0 && conf.message_type == "group") {
        std::wstring msg = trim(wmessage.substr(5));
        std::istringstream iss(wstring_to_string(msg));
        std::string name, index;
        iss >> name >> index;
        return process_del_images(string_to_wstring(name), conf.group_id, index, conf);
    }
    if (wmessage.find(L"美图 删除群 ") == 0 && conf.message_type == "private") {
        std::wstring msg = trim(wmessage.substr(6));
        std::istringstream iss(wstring_to_string(msg));
        groupid_t gid;
        std::string name, index;
        iss >> gid >> name >> index;
        return process_del_images(string_to_wstring(name), gid, index, conf);
    }
    return false;
}

void img::process(std::string message, const msg_meta &conf)
{
    std::wstring wmessage = string_to_wstring(message);
    if (wmessage.find(L"美图 ") == 0) {
        if (process_command(message, conf)) {
            return;
        }
    }
    // TODO: here
    if (std::get<0>(is_adding[conf.user_id]) && wmessage.find(L"[CQ:image,") != std::wstring::npos) {
        add_images(wmessage,
                   wstring_to_string(std::get<2>(is_adding[conf.user_id])),
                   std::get<1>(is_adding[conf.user_id]),
                   conf);
        conf.p->setlog(LOG::INFO, fmt::format("美图 {} add by {}", 
                                              wstring_to_string(std::get<2>(is_adding[conf.user_id])),
                                              std::to_string(conf.user_id)));
        is_adding[conf.user_id] = std::make_tuple(false, 0, L"");
        return;
    }
    if (std::get<0>(is_deling[conf.user_id])) {}

    is_adding[conf.user_id] = std::make_tuple(false, 0, L"");
    is_deling[conf.user_id] = std::make_tuple(false, 0, L"");
    std::string name, indexs;
    groupid_t groupid;
    std::istringstream iss(message);
    try {
        if (conf.message_type == "private") {
            iss >> name >> groupid >> indexs;
            if (!is_in_group(groupid, conf.user_id, conf.p)) {
                return;
            }
        } else {
            iss >> name >> indexs;
            groupid = conf.group_id;
        }
    } catch (...) {
        return;
    }
    // fmt::print("img process name: {}, group: {}, indexs: {}\n", name, groupid, indexs);

    const auto &uuids = uuid_groupid.get_by_second(groupid);
    const auto &it_group = std::find_if(uuids.begin(), uuids.end(), [&](const std::string &u) {
        const auto &names = name_uuid.get_by_second(u);
        return std::find(names.begin(), names.end(), string_to_wstring(name)) != names.end();
    });
    const auto &it_default = std::find_if(default_image_list.begin(), default_image_list.end(), [&](const std::string &u) {
        const auto &names = name_uuid.get_by_second(u);
        return std::find(names.begin(), names.end(), string_to_wstring(name)) != names.end();
    });
    size_t img_size1 = 0, img_size2 = 0, img_size;
    if (it_group != uuids.end() && it_default != default_image_list.end()) {
        img_size1 = image_size[*it_group];
        img_size2 = image_size[*it_default];
        img_size = image_size[*it_group] + image_size[*it_default];
    } else if (it_group != uuids.end()) {
        img_size1 = image_size[*it_group];
        img_size = image_size[*it_group];
    } else if (it_default != default_image_list.end()) {
        img_size2 = image_size[*it_default];
        img_size = image_size[*it_default];
    } else {
        return;
    }
    // fmt::print("img size: {}, {}, {}\n", img_size, img_size1, img_size2);

    Json::Value J;
    J["message_id"] = conf.message_id;
    conf.p->cq_send("mark_msg_as_read", J);
    if (indexs == "all") {
        std::ostringstream oss;
        oss << "转发" << std::endl;
        for (size_t i = 0; i < img_size; ++i) {
            if (i % 10 == 0) {
                oss << fmt::format("{} 第 {} 张", conf.p->get_botqq(), i + 1) << std::endl;
            }
            if (i < img_size1) {
                oss << std::to_string(conf.p->get_botqq())
                    << " [CQ:image,file=file://" << get_local_path()
                    << "/resource/mt/" << *it_group << "/" << i << ",id=40000]"
                    << std::endl;
                continue;
            } else {
                oss << std::to_string(conf.p->get_botqq())
                    << " [CQ:image,file=file://" << get_local_path()
                    << "/resource/mt/" << *it_default << "/" << i - img_size1 << ",id=40000]"
                    << std::endl;
            }
        }

        Json::Value J_send;
        J_send["post_type"] = "message";
        J_send["message"] = oss.str();
        J_send["message_type"] = conf.message_type;
        J_send["message_id"] = conf.message_id;
        J_send["user_id"] = conf.user_id;
        J_send["group_id"] = conf.group_id;
        conf.p->input_process(new std::string(J_send.toStyledString()));
        conf.p->setlog(LOG::INFO, fmt::format("美图 {} all by {}", name,
                                              std::to_string(conf.user_id)));
        return;
    }
    int64_t index;
    if (indexs.length() < 1) {
        index = get_random(img_size) + 1;
    }
    else {
        try {
            index = std::stoi(indexs);
        }
        catch (...) {
            return;
        }
        if (index == 0) {
            return;
        }
    }
    index--;
    if (index < 0 || index >= img_size) {
        conf.p->cq_send("索引越界！(1~" + std::to_string(img_size) + ")",
                        conf);
        return;
    }
    conf.p->setlog(LOG::INFO, "img at group " + std::to_string(conf.group_id));
    if (index < static_cast<int64_t>(img_size1)) {
        conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                        "/resource/mt/" + *it_group + "/" + std::to_string(index) +
                        ",id=40000]",
                    conf);
    } else {
        conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                            "/resource/mt/" + *it_default + "/" + std::to_string(index - img_size1) +
                            ",id=40000]",
                        conf);
    }
}
bool img::check(std::string message, const msg_meta &conf) { return true; }
std::string img::help() { return "美图： 美图 帮助 - 列出所有美图命令"; }

void img::set_backup_files(archivist *p, const std::string &name)
{
    p->add_path(name, "./resource/mt/", "resource/mt/");
}

DECLARE_FACTORY_FUNCTIONS(img)