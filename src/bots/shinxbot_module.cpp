#include "dynamic_lib.hpp"
#include "shinxbot.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = fs;

template <typename T> void close_dl(void *handle, T *p)
{
    typedef void (*close_t)(T *);
    close_t closex = reinterpret_cast<close_t>(dlsym(handle, "destroy_t"));
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        set_global_log(LOG::WARNING,
                       std::string("Cannot load symbol 'destroy_t': ") +
                           dlsym_error);
        // delete p; // This is not always safe
    }
    else {
        closex(p);
    }
    dlclose(handle);
}

void shinxbot::unload_func(std::tuple<processable *, void *, std::string> &f)
{
    this->mytimer->remove_callback(std::get<2>(f));
    this->archive->remove_path(std::get<2>(f));

    close_dl(std::get<1>(f), std::get<0>(f));
}

void shinxbot::unload_func(std::tuple<eventprocess *, void *, std::string> &f)
{
    this->mytimer->remove_callback(std::get<2>(f));
    this->archive->remove_path(std::get<2>(f));
    close_dl(std::get<1>(f), std::get<0>(f));
}

void shinxbot::init_func(const std::string &name, processable *p)
{
    p->set_callback([this, name](std::function<void(bot * p)> func) {
        this->mytimer->add_callback(name, func);
    });
    p->set_backup_files(this->archive, name);
}

void shinxbot::init_func(const std::string &name, eventprocess *p) {}

void shinxbot::load_module_filter_config()
{
    const std::string cfg_path = bot_config_path(this, "core/module_load.json");
    const bool cfg_exists = fs::exists(cfg_path);
    Json::Value J = string_to_json(readfile(cfg_path, "{}"));

    enabled_functions.clear();
    enabled_events.clear();

    const bool has_functions =
        J.isMember("functions") && J["functions"].isArray();
    const bool has_events = J.isMember("events") && J["events"].isArray();
    bool changed = false;

    if (has_functions) {
        parse_json_to_set(J["functions"], enabled_functions);
    }
    else {
        auto fn_names = list_available_module_names(false);
        enabled_functions =
            std::set<std::string>(fn_names.begin(), fn_names.end());
        changed = true;
    }

    if (has_events) {
        parse_json_to_set(J["events"], enabled_events);
    }
    else {
        auto ev_names = list_available_module_names(true);
        enabled_events =
            std::set<std::string>(ev_names.begin(), ev_names.end());
        changed = true;
    }

    if (!cfg_exists || changed) {
        save_module_filter_config();
        set_global_log(LOG::INFO,
                       "Initialized/updated module_load.json: functions=" +
                           std::to_string(enabled_functions.size()) +
                           ", events=" + std::to_string(enabled_events.size()));
    }
}

void shinxbot::save_module_filter_config() const
{
    Json::Value J(Json::objectValue);
    J["functions"] = parse_set_to_json(enabled_functions);
    J["events"] = parse_set_to_json(enabled_events);
    writefile(bot_config_path(this, "core/module_load.json"),
              J.toStyledString());
}

void shinxbot::add_module_to_filter(const std::string &name, bool is_event)
{
    if (is_event) {
        enabled_events.insert(name);
    }
    else {
        enabled_functions.insert(name);
    }
    save_module_filter_config();
}

void shinxbot::remove_module_from_filter(const std::string &name, bool is_event)
{
    if (is_event) {
        enabled_events.erase(name);
    }
    else {
        enabled_functions.erase(name);
    }
    save_module_filter_config();
}

std::vector<std::string>
shinxbot::list_available_module_names(bool is_event) const
{
    std::vector<std::string> names;
    const fs::path dir =
        is_event ? fs::path("./lib/events/") : fs::path("./lib/functions/");
    if (!fs::exists(dir)) {
        return names;
    }

    for (const auto &entry : fs::directory_iterator(dir)) {
        if (!(entry.is_regular_file() || entry.is_symlink())) {
            continue;
        }

        std::string filename = entry.path().filename().string();
        if (filename.size() <= 3 ||
            filename.substr(filename.size() - 3) != ".so") {
            continue;
        }

        std::string name = filename;
        if (name.find("lib") == 0) {
            name = name.substr(3);
        }
        name.erase(name.length() - 3);
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

bool shinxbot::handle_bot_load(const std::string &message, const msg_meta &conf)
{
    std::istringstream iss(message.substr(8));
    std::ostringstream oss;
    std::string type, name;
    iss >> type;

    std::vector<std::string> names;
    while (iss >> name) {
        names.push_back(name);
    }

    if (names.empty()) {
        cq_send("useage: bot.load [function|event] name", conf);
        return false;
    }

    auto contains_name = [](const std::vector<std::string> &arr,
                            const std::string &target) {
        return std::find(arr.begin(), arr.end(), target) != arr.end();
    };

    if (type == "function") {
        const auto available = list_available_module_names(false);
        for (const auto &n : names) {
            if (!contains_name(available, n)) {
                oss << "load " << n
                    << " failed: module not found (use bot.list_alias)"
                    << std::endl;
                continue;
            }

            bool found_loaded = false;
            for (size_t i = 0; i < functions.size(); ++i) {
                if (std::get<2>(functions[i]) == n) {
                    const bool restart_timer = (this->mytimer != nullptr &&
                                                this->mytimer->is_running());
                    if (restart_timer) {
                        this->mytimer->timer_stop();
                    }
                    unload_func(functions[i]);

                    auto u = load_function<processable>("./lib/functions/lib" +
                                                        n + ".so");
                    if (u.first != nullptr) {
                        std::get<0>(functions[i]) = u.first;
                        std::get<1>(functions[i]) = u.second;
                        init_func(n, u.first);
                        add_module_to_filter(n, false);
                        oss << "reload " << n << std::endl;
                    }
                    else {
                        functions.erase(functions.begin() + i);
                        oss << "load " << n << " failed" << std::endl;
                    }
                    if (restart_timer) {
                        this->mytimer->timer_start();
                    }
                    found_loaded = true;
                    break;
                }
            }

            if (!found_loaded) {
                auto u = load_function<processable>("./lib/functions/lib" + n +
                                                    ".so");
                if (u.first != nullptr) {
                    functions.push_back(std::make_tuple(u.first, u.second, n));
                    init_func(n, u.first);
                    add_module_to_filter(n, false);
                    oss << "load " << n << std::endl;
                }
                else {
                    oss << "load " << n << " failed" << std::endl;
                }
            }
        }
        cq_send(trim(oss.str()), conf);
        return false;
    }

    if (type == "event") {
        const auto available = list_available_module_names(true);
        for (const auto &n : names) {
            if (!contains_name(available, n)) {
                oss << "load " << n
                    << " failed: module not found (use bot.list_alias)"
                    << std::endl;
                continue;
            }

            bool found_loaded = false;
            for (size_t i = 0; i < events.size(); ++i) {
                if (std::get<2>(events[i]) == n) {
                    const bool restart_timer = (this->mytimer != nullptr &&
                                                this->mytimer->is_running());
                    if (restart_timer) {
                        this->mytimer->timer_stop();
                    }
                    unload_func(events[i]);

                    auto u = load_function<eventprocess>("./lib/events/lib" +
                                                         n + ".so");
                    if (u.first != nullptr) {
                        std::get<0>(events[i]) = u.first;
                        std::get<1>(events[i]) = u.second;
                        init_func(n, u.first);
                        add_module_to_filter(n, true);
                        oss << "reload " << n << std::endl;
                    }
                    else {
                        events.erase(events.begin() + i);
                        oss << "load " << n << " failed" << std::endl;
                    }
                    if (restart_timer) {
                        this->mytimer->timer_start();
                    }
                    found_loaded = true;
                    break;
                }
            }

            if (!found_loaded) {
                auto u =
                    load_function<eventprocess>("./lib/events/lib" + n + ".so");
                if (u.first != nullptr) {
                    events.push_back(std::make_tuple(u.first, u.second, n));
                    init_func(n, u.first);
                    add_module_to_filter(n, true);
                    oss << "load " << n << std::endl;
                }
                else {
                    oss << "load " << n << " failed" << std::endl;
                }
            }
        }
        cq_send(trim(oss.str()), conf);
        return false;
    }

    cq_send("useage: bot.load [function|event] name", conf);
    return false;
}

bool shinxbot::handle_bot_unload(const std::string &message,
                                 const msg_meta &conf)
{
    std::istringstream iss(message.substr(10));
    std::ostringstream oss;
    std::string type, name;
    iss >> type;

    std::vector<std::string> names;
    while (iss >> name) {
        names.push_back(name);
    }

    if (names.empty()) {
        cq_send("useage: bot.unload [function|event] name", conf);
        return false;
    }

    if (type == "function") {
        for (const auto &n : names) {
            bool flg = true;
            for (size_t i = 0; i < functions.size(); ++i) {
                if (std::get<2>(functions[i]) == n) {
                    const bool restart_timer = (this->mytimer != nullptr &&
                                                this->mytimer->is_running());
                    if (restart_timer) {
                        this->mytimer->timer_stop();
                    }
                    unload_func(functions[i]);
                    functions.erase(functions.begin() + i);
                    if (restart_timer) {
                        this->mytimer->timer_start();
                    }
                    oss << "unload " << n << std::endl;
                    flg = false;
                    break;
                }
            }
            remove_module_from_filter(n, false);
            if (flg) {
                oss << n << " not found (removed from module_load if existed)"
                    << std::endl;
            }
        }
        cq_send(trim(oss.str()), conf);
        return false;
    }

    if (type == "event") {
        for (const auto &n : names) {
            bool flg = true;
            for (size_t i = 0; i < events.size(); ++i) {
                if (std::get<2>(events[i]) == n) {
                    const bool restart_timer = (this->mytimer != nullptr &&
                                                this->mytimer->is_running());
                    if (restart_timer) {
                        this->mytimer->timer_stop();
                    }
                    unload_func(events[i]);
                    events.erase(events.begin() + i);
                    if (restart_timer) {
                        this->mytimer->timer_start();
                    }
                    oss << "unload " << n << std::endl;
                    flg = false;
                    break;
                }
            }
            remove_module_from_filter(n, true);
            if (flg) {
                oss << n << " not found (removed from module_load if existed)"
                    << std::endl;
            }
        }
        cq_send(trim(oss.str()), conf);
        return false;
    }

    cq_send("useage: bot.unload [function|event] name", conf);
    return false;
}

bool shinxbot::handle_bot_reload(const std::string &message,
                                 const msg_meta &conf)
{
    std::istringstream iss(message.substr(10));
    std::ostringstream oss;
    std::string type;
    iss >> type;

    std::vector<std::string> names;
    std::string name;
    while (iss >> name) {
        names.push_back(name);
    }

    if (type != "function") {
        cq_send("useage: bot.reload function [name|all]", conf);
        return false;
    }

    if (functions.empty()) {
        cq_send("No loaded functions.", conf);
        return false;
    }

    auto reload_one = [&](processable *func, const std::string &alias) {
        bool ok = false;
        try {
            ok = func->reload(conf);
        }
        catch (...) {
            ok = false;
        }

        if (ok) {
            oss << "reload " << alias << " ok" << std::endl;
        }
        else {
            oss << "reload " << alias << " skipped (stateless or unsupported)"
                << std::endl;
        }
    };

    if (names.empty() || (names.size() == 1 && names[0] == "all")) {
        for (const auto &u : functions) {
            reload_one(std::get<0>(u), std::get<2>(u));
        }
        cq_send(trim(oss.str()), conf);
        return false;
    }

    for (const auto &target : names) {
        bool found = false;
        for (const auto &u : functions) {
            if (std::get<2>(u) == target) {
                reload_one(std::get<0>(u), target);
                found = true;
                break;
            }
        }
        if (!found) {
            oss << "reload " << target << " failed: not loaded" << std::endl;
        }
    }

    cq_send(trim(oss.str()), conf);
    return false;
}
