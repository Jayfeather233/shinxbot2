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

2. Run cppcheck (focused paths):

```bash
cppcheck src/bots src/meta_event src/interfaces src/utils src/functions/memberchangef \
  --enable=warning,style,performance,portability --inconclusive --std=c++17 \
  --suppress=missingIncludeSystem --suppress=preprocessorErrorDirective --quiet
```

3. Run clang-tidy (selected files):

```bash
clang-tidy src/bots/shinxbot.cpp -p build
```

4. Rebuild and verify:

```bash
./build_features.sh
./build.sh
```

## Commit Scope Guidance

- Keep commits theme-based:
  - behavior fix
  - safety/lifecycle hardening
  - style/perf cleanup
  - tooling/docs
- Do not mix unrelated behavior changes with style-only edits in one commit.
