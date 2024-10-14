#include "utils.h"

#include "httplib.h"
#include <iostream>
#include <jsoncpp/json/json.h>
#include <mutex>

enum class http_req_method { GET, POST }; // for now, these are enough

std::string do_http_request(const std::string &httpaddr,
                            const std::map<std::string, std::string> &headers,
                            const bool proxy_flg, const http_req_method hrm,
                            const Json::Value &json_message = Json::Value())
{
    httplib::Client client(httpaddr.c_str());
    client.set_connection_timeout(0, 5000000); // 5 seconds
    client.set_read_timeout(5, 0);             // 5 seconds
    client.set_write_timeout(5, 0);            // 5 seconds
    if (proxy_flg) {
        const char *http_proxy = std::getenv("http_proxy");
        if (!http_proxy) {
            http_proxy = std::getenv("HTTP_PROXY");
        }
        if (http_proxy) {
            std::string proxy_str(http_proxy);
            size_t colon_pos = proxy_str.find(':');
            if (colon_pos != std::string::npos) {
                std::string proxy_host = proxy_str.substr(0, colon_pos);
                int proxy_port = std::stoi(proxy_str.substr(colon_pos + 1));
                client.set_proxy(proxy_host.c_str(), proxy_port);
            }
        } else {
            set_global_log(LOG::WARNING, fmt::format("HTTP Request: {} need proxy but no system proxy found.", httpaddr));
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
            ? client.Post("", httplib_headers, json_message.toStyledString(),
                          "application/json")
            : client.Get("", httplib_headers);

    if (!res || res->status != 200) {
        auto err = res.error();
        set_global_log(LOG::ERROR,
                       fmt::format("Connect to {} failed with code {} err: {}",
                                   std::to_string(res ? res->status : -1),
                                   httpaddr, httplib::to_string(err)));
        throw fmt::format("HTTP Connect failed, err {}", httplib::to_string(err));
    }

    return res->body;
}

std::string do_post(const std::string &httpaddr,
                    const Json::Value &json_message,
                    const std::map<std::string, std::string> &headers,
                    const bool proxy_flg)
{
    return do_http_request(httpaddr, headers, proxy_flg, http_req_method::POST, json_message);
}

std::string do_get(const std::string &httpaddr,
                   const std::map<std::string, std::string> &headers,
                   const bool proxy_flg)
{
    return do_http_request(httpaddr, headers, proxy_flg, http_req_method::GET);
}
