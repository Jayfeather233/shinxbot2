#include "utils.h"

bool cmd_match_exact(const std::string &message,
                     const std::vector<std::string> &commands)
{
    for (const auto &cmd : commands) {
        if (message == cmd) {
            return true;
        }
    }
    return false;
}

bool cmd_match_exact(const std::wstring &message,
                     const std::vector<std::wstring> &commands)
{
    for (const auto &cmd : commands) {
        if (message == cmd) {
            return true;
        }
    }
    return false;
}

bool cmd_match_prefix(const std::string &message,
                      const std::vector<std::string> &prefixes)
{
    for (const auto &prefix : prefixes) {
        if (starts_with(message, prefix)) {
            return true;
        }
    }
    return false;
}

bool cmd_match_prefix(const std::wstring &message,
                      const std::vector<std::wstring> &prefixes)
{
    for (const auto &prefix : prefixes) {
        if (starts_with(message, prefix)) {
            return true;
        }
    }
    return false;
}

bool cmd_strip_prefix(const std::string &message, const std::string &prefix,
                      std::string &body_out)
{
    if (!starts_with(message, prefix)) {
        return false;
    }
    body_out = trim(message.substr(prefix.size()));
    return true;
}

bool cmd_strip_prefix(const std::wstring &message, const std::wstring &prefix,
                      std::wstring &body_out)
{
    if (!starts_with(message, prefix)) {
        return false;
    }
    body_out = trim(message.substr(prefix.size()));
    return true;
}

bool cmd_parse_prefixed(const std::string &raw,
                        const std::vector<std::string> &prefixes,
                        std::string &cmd_out)
{
    const std::string m = trim(raw);
    for (const auto &prefix : prefixes) {
        if (cmd_strip_prefix(m, prefix, cmd_out)) {
            return true;
        }
    }
    return false;
}

bool cmd_parse_prefixed(const std::wstring &raw,
                        const std::vector<std::wstring> &prefixes,
                        std::wstring &cmd_out)
{
    const std::wstring m = trim(raw);
    for (const auto &prefix : prefixes) {
        if (cmd_strip_prefix(m, prefix, cmd_out)) {
            return true;
        }
    }
    return false;
}

bool cmd_run_middlewares(const std::vector<cmd_middleware_t> &middlewares)
{
    for (const auto &mw : middlewares) {
        if (!mw || !mw()) {
            return false;
        }
    }
    return true;
}

bool cmd_dispatch(const std::string &message,
                  const std::vector<cmd_exact_rule> &exact_rules,
                  const std::vector<cmd_prefix_rule> &prefix_rules)
{
    bool handled = false;
    (void)cmd_try_dispatch(message, exact_rules, prefix_rules, handled);
    return handled;
}

bool cmd_try_dispatch(const std::string &message,
                      const std::vector<cmd_exact_rule> &exact_rules,
                      const std::vector<cmd_prefix_rule> &prefix_rules,
                      bool &handled_out)
{
    handled_out = false;
    for (const auto &r : exact_rules) {
        if (message == r.cmd) {
            handled_out = true;
            if (!cmd_run_middlewares(r.middlewares)) {
                return true;
            }
            return r.handler ? r.handler() : true;
        }
    }
    for (const auto &r : prefix_rules) {
        if (starts_with(message, r.prefix)) {
            handled_out = true;
            if (!cmd_run_middlewares(r.middlewares)) {
                return true;
            }
            return r.handler ? r.handler() : true;
        }
    }
    return false;
}
