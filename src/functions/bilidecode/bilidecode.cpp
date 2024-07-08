/**
 * This code using API from Bilibili.
 * Decode by
 * https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/docs/video/videostream_url.md
 */

#include "bilidecode.h"

av_result bili_decode::get_av(std::string s, size_t pos)
{
    pos = s.find("av", pos);
    if (pos == s.npos) {
        return std::make_pair(0, pos);
    }
    pos += 2;
    uint64_t avid = 0;
    while (pos < s.length() && is_digit(s[pos])) {
        avid = (avid << 3) + (avid << 1) + s[pos] - '0';
        ++pos;
    }
    return std::make_pair(avid, pos);
}

bv_result bili_decode::get_bv(std::string s, size_t pos)
{
    pos = s.find("BV", pos);
    if (pos == s.npos) {
        return std::make_pair(std::string(), pos);
    }
    std::string bvid;
    while (pos < s.length() && (is_digit(s[pos]) || is_word(s[pos]))) {
        bvid += s[pos];
        ++pos;
    }
    return std::make_pair(bvid, pos);
}
void bili_decode::send_dec_info(const Json::Value &J, const msg_meta &conf)
{
    conf.p->cq_send(get_decode_info(J), conf);
    conf.p->setlog(LOG::INFO, "bilidecoder: group " +
                                  std::to_string(conf.group_id) + " user " +
                                  std::to_string(conf.user_id) +
                                  J["data"]["bvid"].asString());
}

void bili_decode::process_string(std::string s, const msg_meta &conf)
{
    {
        bv_result res = get_bv(s);
        if (res.first.empty())
            return;
        Json::Value raw_info = get_raw_info(res.first);
        if (raw_info["code"].asInt64() == 0) {
            send_dec_info(raw_info, conf);
            return;
        }
    }
    {
        av_result res = get_av(s);
        if (res.first == 0)
            return;
        Json::Value raw_info = get_raw_info(res.first);
        if (raw_info["code"].asInt64() == 0) {
            send_dec_info(raw_info, conf);
            return;
        }
    }
}

void bili_decode::process(Json::Value messageArr, const msg_meta &conf)
{
    Json::ArrayIndex sz = messageArr.size();
    for (Json::ArrayIndex i = 0; i < sz; ++i) {
        if (messageArr[i]["type"] == "text") {
            process_string(messageArr[i]["data"]["text"].asString(), conf);
        }
    }
}

bool bili_decode::check(Json::Value message, const msg_meta &conf)
{
    return true;
}

Json::Value bili_decode::get_raw_info(uint64_t aid)
{
    return string_to_json(
        do_get("https://api.bilibili.com/x/web-interface/view?aid=" +
               std::to_string(aid)));
}
Json::Value bili_decode::get_raw_info(std::string bvid)
{
    return string_to_json(
        do_get("https://api.bilibili.com/x/web-interface/view?bvid=" + bvid));
}
std::string bili_decode::get_decode_info(const Json::Value &raw_info)
{
    std::ostringstream oss;
    oss << "[CQ:image,file=" << raw_info["data"]["pic"].asString()
        << ",id=40000]\n";
    oss << raw_info["data"]["bvid"].asString() << " 分区："
        << raw_info["data"]["tname"].asString() << std::endl;
    oss << "标题：" << raw_info["data"]["title"].asString() << std::endl;
    std::wstring desc = string_to_wstring(raw_info["data"]["desc"].asString());
    if (desc.length() >= 100) {
        desc = desc.substr(0, 100) + L"...";
    }
    oss << "简介：" << wstring_to_string(desc) << std::endl;

    oss << "UP: " << raw_info["data"]["owner"]["name"].asString() << std::endl;
    oss << "播放 "
        << to_human_string(raw_info["data"]["stat"]["view"].asInt64())
        << " 点赞 "
        << to_human_string(raw_info["data"]["stat"]["like"].asInt64())
        << " 回复 "
        << to_human_string(raw_info["data"]["stat"]["reply"].asInt64())
        << " 弹幕 "
        << to_human_string(raw_info["data"]["stat"]["danmaku"].asInt64())
        << std::endl;
    oss << "Link: https://www.bilibili.com/video/" +
               raw_info["data"]["bvid"].asString() + "/"
        << std::endl;

    return trim(oss.str());

    // ["bvid"]     str -> bvid
    // ["tname"]    str -> 子分区名
    // ["pic"]      str -> cover_http
    // ["title"]    str -> title
    // ["desc"]     str -> jian jie
    // ["owner"]["name"]    str -> up name
    // ["stat"]["view"]     int ->
    // ["stat"]["danmaku"]  int ->
    // ["stat"]["reply"]    int ->
    // ["stat"]["like"]     int ->
}
std::string bili_decode::help() { return "对av和BV号，下载封面图和视频数据"; }

bool bili_decode::is_support_messageArr() { return true; }

/// deprecated below

void bili_decode::process(std::string message, const msg_meta &conf)
{
    Json::Value raw_info;
    bool flg = false;
    size_t u;
    if ((u = message.find("BV")) != message.npos) {
        if (u != 0 && is_word(message[u - 1])) {
            return;
        }
        Json::Value J;
        J["message_id"] = conf.message_id;
        conf.p->cq_send("mark_msg_as_read", J);
        flg = true;
        bv_result res = get_bv(message);
        std::string bvid = res.first;
        size_t pos = res.second;
        res = get_bv(message, pos);
        pos = res.second;
        while (flg && pos != message.npos) {
            if (bvid != res.first)
                flg = false;
            res = get_bv(message, pos);
            pos = res.second;
        }
        if (flg) {
            raw_info = get_raw_info(bvid);
        }
    }
    else if ((u = message.find("av")) != message.npos) {
        if (u != 0 && is_word(message[u - 1])) {
            return;
        }
        Json::Value J;
        J["message_id"] = conf.message_id;
        conf.p->cq_send("mark_msg_as_read", J);
        flg = true;
        av_result res = get_av(message);
        uint64_t avid = res.first;
        size_t pos = res.second;
        res = get_av(message, pos);
        pos = res.second;
        while (flg && pos != message.npos) {
            if (avid != res.first)
                flg = false;
            res = get_av(message, pos);
            pos = res.second;
        }
        if (flg) {
            raw_info = get_raw_info(avid);
        }
    }
    if (flg && raw_info["code"].asInt64() == 0) {
        conf.p->cq_send(get_decode_info(raw_info), conf);

        conf.p->setlog(LOG::INFO, "bilidecoder: group " +
                                      std::to_string(conf.group_id) + " user " +
                                      std::to_string(conf.user_id) +
                                      raw_info["data"]["bvid"].asString());
    }
}
bool bili_decode::check(std::string message, const msg_meta &conf)
{
    return true;
}

extern "C" processable* create() {
    return new bili_decode();
}