#include "utils.h"

#include "httplib.h"
#include <fmt/ostream.h>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <mutex>

enum class http_req_method { GET, POST }; // for now, these are enough

std::string do_http_request(httplib::Client &client,
                            const std::string &httpaddr,
                            const std::string &httppath,
                            const std::map<std::string, std::string> &headers,
                            const bool proxy_flg, const http_req_method hrm,
                            const Json::Value &json_message = Json::Value())
{

    client.set_connection_timeout(10, 0); // 10 seconds
    client.set_read_timeout(10, 0);       // 10 seconds
    client.set_write_timeout(10, 0);      // 10 seconds
    if (proxy_flg) {
        const char *http_proxy = std::getenv("http_proxy");
        if (!http_proxy) {
            http_proxy = std::getenv("HTTP_PROXY");
        }
        if (http_proxy) {
            try {
                std::string proxy_str(http_proxy);
                size_t colon_pos = proxy_str.find(':');
                if (proxy_str[colon_pos + 1] == '/') {
                    colon_pos = proxy_str.find(':', colon_pos + 1);
                }
                if (colon_pos != std::string::npos) {
                    std::string proxy_host = proxy_str.substr(0, colon_pos);
                    int proxy_port = std::stoi(proxy_str.substr(colon_pos + 1));
                    client.set_proxy(proxy_host, proxy_port);
                }
            }
            catch (...) {
                set_global_log(LOG::WARNING, "Unable to resolve proxy");
            }
        }
        else {
            set_global_log(
                LOG::WARNING,
                fmt::format(
                    "HTTP Request: {} need proxy but no system proxy found.",
                    httpaddr));
        }
    }

    // Set headers
    httplib::Headers httplib_headers;
    for (const auto &header : headers) {
        httplib_headers.emplace(header.first, header.second);
    }
    // httplib_headers.emplace("Content-Type", "application/json");

    // Perform the POST request
    httplib::Result res =
        hrm == http_req_method::POST
            ? client.Post(httppath, httplib_headers,
                          json_message.toStyledString(), "application/json")
            : client.Get(httppath, httplib_headers);

    if (!res || res->status / 100 != 2) {
        auto err = res.error();
        std::cout << (res ? res->body : "No response") << std::endl;
        set_global_log(LOG::ERROR,
                       fmt::format("Connect to {} failed with code {}, err: {}",
                                   httpaddr + httppath,
                                   std::to_string(res ? res->status : -1),
                                   httplib::to_string(err)));
        throw fmt::format("HTTP Connect failed, err {}",
                          httplib::to_string(err));
    }

    return res->body;
}

std::string do_post(const std::string &httpaddr,
                    const Json::Value &json_message,
                    const std::map<std::string, std::string> &headers,
                    const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr);
    return do_http_request(cli, httpaddr, "", headers, proxy_flg,
                           http_req_method::POST, json_message);
}

std::string do_get(const std::string &httpaddr,
                   const std::map<std::string, std::string> &headers,
                   const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr);
    return do_http_request(cli, httpaddr, "", headers, proxy_flg,
                           http_req_method::GET);
}

std::string do_post(const std::string &httpaddr, const std::string &httppath,
                    bool enc, const Json::Value &json_message,
                    const std::map<std::string, std::string> &headers,
                    const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr);
    cli.set_path_encode(enc);
    return do_http_request(cli, httpaddr, httppath, headers, proxy_flg,
                           http_req_method::POST, json_message);
}

std::string do_get(const std::string &httpaddr, const std::string &httppath,
                   bool enc, const std::map<std::string, std::string> &headers,
                   const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr);
    cli.set_path_encode(enc);
    return do_http_request(cli, httpaddr, httppath, headers, proxy_flg,
                           http_req_method::GET);
}

std::string do_post(const std::string &httpaddr, int port,
                    const Json::Value &json_message,
                    const std::map<std::string, std::string> &headers,
                    const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr, port);
    return do_http_request(cli, fmt::format("{}:{}", httpaddr, port), "",
                           headers, proxy_flg, http_req_method::POST,
                           json_message);
}

std::string do_get(const std::string &httpaddr, int port,
                   const std::map<std::string, std::string> &headers,
                   const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr, port);
    return do_http_request(cli, fmt::format("{}:{}", httpaddr, port), "",
                           headers, proxy_flg, http_req_method::GET);
}

std::string do_post(const std::string &httpaddr, int port,
                    const std::string &httppath, bool enc,
                    const Json::Value &json_message,
                    const std::map<std::string, std::string> &headers,
                    const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr, port);
    cli.set_path_encode(enc);
    return do_http_request(cli, fmt::format("{}:{}", httpaddr, port), httppath,
                           headers, proxy_flg, http_req_method::POST,
                           json_message);
}

std::string do_get(const std::string &httpaddr, int port,
                   const std::string &httppath, bool enc,
                   const std::map<std::string, std::string> &headers,
                   const bool proxy_flg)
{
    auto cli = httplib::Client(httpaddr, port);
    cli.set_path_encode(enc);
    return do_http_request(cli, fmt::format("{}:{}", httpaddr, port), httppath,
                           headers, proxy_flg, http_req_method::GET);
}