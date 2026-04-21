# Plugin Development Spec (shinxbot2)

This document defines the runtime contract for function/event plugins and provides a practical checklist for code reviews.

## 1. Function Plugin Contract

A function plugin must inherit `processable` and implement:

- `bool check(std::string, const msg_meta&)`
- `void process(std::string, const msg_meta&)`
- `std::string help()`

Optional (recommended when needed):

- `std::string help(const msg_meta&, help_level_t)` for privilege-aware help output.
- `bool reload(const msg_meta&)` for hot-reload state refresh.
- `void set_callback(...)` for timer callback registration.
- `void set_backup_files(...)` for archive path registration.
- message-array mode:
  - `bool is_support_messageArr()` returns true
  - `bool check(Json::Value, const msg_meta&)`
  - `void process(Json::Value, const msg_meta&)`

## 2. Event Plugin Contract

An event plugin must inherit `eventprocess` and implement:

- `bool check(bot*, Json::Value)`
- `void process(bot*, Json::Value)`

Rules:

- `check()` must verify required fields and payload type (`post_type`, `notice_type`, etc.).
- `check()` should be side-effect free.
- `process()` should assume `check()` already validated minimum schema.
- For OneBot notice events, prefer checking `post_type=notice` and exact
  `notice_type/sub_type` combos to avoid accidental matches.

## 3. Dynamic Library Export Contract

All plugins must export the factory symbols below:

- `create_t`
- `destroy_t`

Use the macro in implementation file:

- function plugin: `DECLARE_FACTORY_FUNCTIONS(YourClass)` from `processable.h`
- event plugin: `DECLARE_FACTORY_FUNCTIONS(YourClass)` from `eventprocess.h`

Do not export legacy `create` / `close` symbols for new modules.

## 4. Command Routing Rules (Functions)

Use shared helpers from `utils.h`:

- `cmd_try_dispatch` / `cmd_dispatch`
- `cmd_strip_prefix`
- `cmd_match_exact` / `cmd_match_prefix`

Guidelines:

- Keep `check()` and `process()` command domains aligned.
- Prefer exact rules for singleton commands.
- For command families, use trailing-space prefixes (e.g. `"*foo "`) to avoid over-matching.
- Put authorization checks in middleware where possible.
- For unknown commands, send explicit usage/help text.

## 5. Help and Permission Model

Help levels:

- `public_only`
- `group_admin`
- `bot_admin`

Recommendations:

- `help()` should be concise and safe for public contexts.
- privileged commands should appear only in elevated help.
- bot-level operator commands should be visible only in private OP contexts.

## 6. State and Path Management

Use:

- `bot_config_path(...)`
- `bot_resource_path(...)`

Avoid hardcoded runtime paths in plugins.

For mutable state:

- guard shared mutable state with mutex where needed.
- keep `reload()` idempotent.
- avoid detached threads in plugin lifetime unless explicitly managed.

## 7. Plugin Review Checklist

- [ ] `check()` is strict and side-effect free.
- [ ] command routes are reachable from `check()`.
- [ ] no broad prefix over-match for unrelated commands.
- [ ] help text is layered by permission.
- [ ] path access uses helper functions.
- [ ] factory exports are `create_t` and `destroy_t`.
- [ ] reload behavior is safe for hot-reload paths.

## 8. One-Command Module Bootstrap

Function module skeleton:

```bash
cd src/functions
python3 generate_cmake.py --init your_module_name
```

Event module skeleton:

```bash
cd src/events
python3 generate_cmake.py --init your_event_name
```

Legacy batch mode for existing module directories remains unchanged:

```bash
python3 generate_cmake.py
```
