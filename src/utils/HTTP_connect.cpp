#include "utils.h"

#include "httplib.h"
#include <cctype>
#include <fmt/ostream.h>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <mutex>

enum class http_req_method { GET, POST }; // for now, these are enough

static std::string summarize_body_for_log(const std::string &body)
{
    if (body.empty()) {
        return "empty";
    }

    const bool looks_html = body.find("<!DOCTYPE html") != std::string::npos ||
                            body.find("<html") != std::string::npos ||
                            body.find("<noscript") != std::string::npos;
    if (looks_html) {
        return fmt::format("[html body omitted, {} bytes]", body.size());
    }

    std::string flat;
    flat.reserve(std::min<size_t>(body.size(), 192));
    bool last_space = false;
    for (char ch : body) {
        unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isspace(uch)) {
            if (!last_space) {
                flat.push_back(' ');
                last_space = true;
            }
            continue;
        }

        flat.push_back(std::isprint(uch) ? ch : '?');
        last_space = false;
        if (flat.size() >= 180) {
            break;
        }
    }

    if (flat.empty()) {
        flat = "[non-printable body]";
    }
    if (body.size() > 180) {
        flat += "...";
    }
    return flat;
}

std::string do_http_request(httplib::Client &client,
                            const std::string &httpaddr,
                            const std::string &httppath,
                            const std::map<std::string, std::string> &headers,
                            const bool proxy_flg, const http_req_method hrm,
                            const Json::Value &json_message = Json::Value())
{

    client.set_connection_timeout(600, 0); // 10 minutes
    client.set_read_timeout(600, 0);       // 10 minutes
    client.set_write_timeout(600, 0);      // 10 minutes
    if (proxy_flg) {
        auto get_env = [](const char *k1, const char *k2) -> const char * {
            const char *v = std::getenv(k1);
            return v ? v : std::getenv(k2);
        };

        const bool is_https = httpaddr.rfind("https://", 0) == 0;
        const char *proxy_env = is_https ? get_env("https_proxy", "HTTPS_PROXY")
                                         : get_env("http_proxy", "HTTP_PROXY");
        if (!proxy_env) {
            proxy_env = get_env("http_proxy", "HTTP_PROXY");
        }

        if (proxy_env) {
            try {
                std::string proxy_str(proxy_env);

                const size_t scheme_pos = proxy_str.find("://");
                if (scheme_pos != std::string::npos) {
                    proxy_str = proxy_str.substr(scheme_pos + 3);
                }

                const size_t slash_pos = proxy_str.find('/');
                if (slash_pos != std::string::npos) {
                    proxy_str = proxy_str.substr(0, slash_pos);
                }

                const size_t at_pos = proxy_str.rfind('@');
                if (at_pos != std::string::npos) {
                    proxy_str = proxy_str.substr(at_pos + 1);
                }

                std::string proxy_host;
                std::string proxy_port_str;
                if (!proxy_str.empty() && proxy_str[0] == '[') {
                    const size_t close = proxy_str.find(']');
                    if (close != std::string::npos) {
                        proxy_host = proxy_str.substr(1, close - 1);
                        if (close + 2 < proxy_str.size() &&
                            proxy_str[close + 1] == ':') {
                            proxy_port_str = proxy_str.substr(close + 2);
                        }
                    }
                }
                else {
                    const size_t colon_pos = proxy_str.rfind(':');
                    if (colon_pos != std::string::npos) {
                        proxy_host = proxy_str.substr(0, colon_pos);
                        proxy_port_str = proxy_str.substr(colon_pos + 1);
                    }
                }

                if (!proxy_host.empty() && !proxy_port_str.empty()) {
                    const int proxy_port = std::stoi(proxy_port_str);
                    client.set_proxy(proxy_host, proxy_port);
                }
                else {
                    set_global_log(LOG::WARNING,
                                   "Unable to resolve proxy host/port");
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
        const int status = res ? res->status : -1;
        const std::string body = res ? res->body : "No response";
        const std::string body_head = summarize_body_for_log(body);

        set_global_log(
            LOG::ERROR,
            fmt::format(
                "Connect to {} failed with code {}, err: {}, body_head: {}",
                httpaddr + httppath, status, httplib::to_string(err),
                body_head));
        throw fmt::format(
            "HTTP request failed: status={} err={} body_head='{}'", status,
            httplib::to_string(err), body_head);
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