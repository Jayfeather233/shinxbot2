#include "shinxbot.hpp"

#include <fmt/core.h>
#include <httplib.h>
#include <thread>

int shinxbot::start_server()
{
    httplib::Server svr;

    svr.Post("/", [&](const httplib::Request &req, httplib::Response &res) {
        std::string s_buffer = req.body;
        input_process(s_buffer);

        res.set_content("{}", "application/json");
    });

    set_global_log(LOG::INFO, fmt::format("Server is starting on port {}...",
                                          receive_port));
    return svr.listen("0.0.0.0", receive_port);
}
