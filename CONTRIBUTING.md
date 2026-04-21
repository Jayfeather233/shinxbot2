# Contributing Guide

This document records current naming/style conventions and practical engineering rules used in this repository.

## Style Baseline

- Language: C++17 (primary), Bash, Python tooling scripts.
- Keep external behavior stable when doing cleanup/refactor.
- Prefer small, focused patches over broad rewrites.
- Avoid changing command names, API payloads, and user-visible text unless required.

## C++ Conventions

- Naming:
  - `snake_case` for functions/variables.
  - `CamelCase` for classes/types.
  - `kPascalCase` for constants where already used.
- Paths:
  - Use `bot_config_path(...)` / `bot_resource_path(...)` helpers.
  - Avoid new hardcoded `./config` / `./resource` literals in runtime logic.
- Resource handling:
  - Prefer RAII and standard containers.
  - Avoid detached threads for module-local lifetime work.
  - If background logic is needed, prefer managed timer callbacks or joinable threads with explicit stop.
- Error handling:
  - Catch by reference (`const std::exception &e`).
  - Re-throw with `throw;` when propagating current exception.

## Runtime and Safety Rules

- Do not introduce object-lifetime races between detached threads and unload/destruct paths.
- Initialize class members explicitly in constructors/member initializers.
- Keep logging paths and backup paths consistent with helper-based directory abstraction.

## Function/Event Plugin Contract

- Function plugin (`processable`) minimum contract:
  - Implement `check(std::string, const msg_meta&)` and
    `process(std::string, const msg_meta&)`.
  - Implement `help()` (can be concise) and optionally
    `help(const msg_meta&, help_level_t)` for privilege-aware help.
- Event plugin (`eventprocess`) minimum contract:
  - Implement `check(bot*, Json::Value)` and `process(bot*, Json::Value)`.
  - `check` should validate all required fields and return `false` for partial
    payloads.
- Shared object export contract (required):
  - Export `extern "C" <type>* create_t()` and
    `extern "C" void destroy_t(<type>*)`.
  - Prefer `DECLARE_FACTORY_FUNCTIONS(YourClass)` macro in plugin `.cpp`.
- Optional function hooks (use only when needed):
  - `reload(const msg_meta&)` for hot-reload state refresh.
  - `set_callback(...)` for timer callbacks.
  - `set_backup_files(...)` to register module data paths.
  - `is_support_messageArr()/check(Json::Value)/process(Json::Value)` for
    message-array processing.

## Command Routing Contract

- Use `cmd_try_dispatch`/`cmd_dispatch` plus exact/prefix rules in preference
  to ad-hoc parsing.
- Keep `check()` and `process()` command domains aligned:
  - every command handled in `process()` must be reachable via `check()`.
- Prefer exact matching for singleton commands and trailing-space prefixes for
  command families (e.g. `"cmd "`) to avoid accidental over-match.
- Middleware should do authorization/filtering only; keep side effects in
  handlers.
- Unknown command behavior should be explicit (usage/help message), not silent.

## Plugin Review Checklist

- `check` is side-effect free and strict about required payload fields.
- `process` uses utilities from `utils.h` for prefix parsing and CQ processing.
- All config/resource paths go through `bot_config_path`/`bot_resource_path`.
- Help text is available at least at public level; privileged commands are
  hidden from public help.
- Dynamic library exports use `create_t`/`destroy_t` only.

## Shell Script Conventions

- Start scripts with:

```bash
set -euo pipefail
```

- Use `mkdir -p` for idempotent directory creation.
- Quote variable expansions unless intentional word splitting is required.
- Prefer `cmake -S ... -B ...` and `cmake --build ...` for portability.

## Docker Conventions

- Use stable base image tags (`debian:12` currently).
- Prefer distro packages over source builds unless version pinning requires otherwise.
- On Debian 12, `imagemagick`/`libmagick++-dev` are ImageMagick 6.x; if 7.x is required, install it manually and document the reason.
- Use `--no-install-recommends` and clean apt lists to reduce image size.
- Keep compose schema modern (`3.8`) and set restart policy where appropriate.

## Static Analysis Workflow

Use this sequence for cleanup with behavior preservation:

1. Generate compile database:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

1. Run cppcheck (focused paths):

```bash
cppcheck src/bots src/meta_event src/interfaces src/utils src/functions/memberchangef \
  --enable=warning,style,performance,portability --inconclusive --std=c++17 \
  --suppress=missingIncludeSystem --suppress=preprocessorErrorDirective --quiet
```

1. Run clang-tidy (selected files):

```bash
clang-tidy src/bots/shinxbot.cpp -p build
```

1. Rebuild and verify:

```bash
./build_features.sh
./build.sh

1. Run command-routing regression checks:

```bash
python3 tests/regression/test_command_routing.py
```

## Module Bootstrap Workflow

- Function module one-shot scaffold:

```bash
cd src/functions
python3 generate_cmake.py --init your_module_name
```

- Event module one-shot scaffold:

```bash
cd src/events
python3 generate_cmake.py --init your_event_name
```

- Batch regenerate CMakeLists for existing modules (legacy mode):

```bash
python3 generate_cmake.py
```

## Commit Scope Guidance

- Keep commits theme-based:
  - behavior fix
  - safety/lifecycle hardening
  - style/perf cleanup
  - tooling/docs
- Do not mix unrelated behavior changes with style-only edits in one commit.
