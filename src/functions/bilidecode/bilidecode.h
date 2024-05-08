#pragma once

#include "processable.h"

typedef std::pair<uint64_t, size_t> av_result;
typedef std::pair<std::string, size_t> bv_result;

class bili_decode : public processable {
private:
    Json::Value get_raw_info(uint64_t aid);
    Json::Value get_raw_info(std::string bvid);
    std::string get_decode_info(const Json::Value &raw_info);
    av_result get_av(std::string s, size_t pos = 0);
    bv_result get_bv(std::string s, size_t pos = 0);
    void process_string(std::string s, const msg_meta &conf);
    void send_dec_info(const Json::Value &J, const msg_meta &conf);

public:
    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    bool is_support_messageArr();
    void process(Json::Value message, const msg_meta &conf);
    bool check(Json::Value message, const msg_meta &conf);
};