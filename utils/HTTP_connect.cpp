#include "utils.h"

std::string do_post(std::string url, std::string endpoint, Json::Value json_message, std::map<std::string, std::string> headers) {
    httplib::Client client(url);

    // add custom headers
    httplib::Headers header = {
        { "Content-Type", "application/json" }
    };
    for (auto it : headers) {
        header.insert(it);
    }

    // set the request body
    std::string body = json_message.toStyledString();

    // send the POST request and get the response
    auto res = client.Post("/" + endpoint, header, body, "application/json");

    // return the response body
    if(res){
        return res->body;
    } else {
        setlog(LOG::ERROR, httplib::to_string(res.error()));
        throw "http connect failed";
    }
}

std::string do_post(std::string url, Json::Value J, std::map<std::string, std::string> header) {
    httplib::Client cli(url);

    // Convert the JSON value to a string
    std::string body = J.toStyledString();

    // Set the content type to JSON
    httplib::Headers headers = {
        { "Content-Type", "application/json" }
    };
    for (auto it : header) {
        headers.insert(it);
    }

  // Perform the POST request and get the response
    auto res = cli.Post("", headers, body, "application/json");

    // Check if the request was successful
    if (res && res->status == 200) {
        return res->body;
    } else {
        setlog(LOG::ERROR, httplib::to_string(res.error()));
        throw std::runtime_error("POST request failed");
    }
}

std::string do_get(std::string url, std::string endpoint, std::map<std::string, std::string> headers) {
    httplib::Client client(url);
    // add custom headers
    httplib::Headers header;
    for (auto it : headers) {
        header.insert(it);
    }

    // send the POST request and get the response
    auto res = client.Get("/" + endpoint, header);

    // return the response body
    if(res){
        return res->body;
    } else {
        setlog(LOG::ERROR, httplib::to_string(res.error()));
        throw "http connect failed";
    }
}

std::string do_get(std::string url, std::map<std::string, std::string> headers) {
    // create HTTP request object
    httplib::Client client(url);
    
    // add custom headers
    httplib::Headers header;
    for (auto it : headers) {
        header.insert(it);
    }

    // send the Get request and get the response
    auto res = client.Get("", header);

    // return the response body
    if(res){
        return res->body;
    } else {
        setlog(LOG::ERROR, httplib::to_string(res.error()));
        throw "http connect failed";
    }
}
