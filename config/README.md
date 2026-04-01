# Config Layout Notes

This directory is organized by responsibility to keep runtime behavior and data ownership clear.

## Recommended Grouping

- `config/core/`: bot runtime controls and operator policy
  - `op_list.json`, `blocklist.json`, `module_load.json`, `recover.json`
- `config/features/<feature>/`: per-feature config and runtime state (co-located)
  - examples:
    - `config/features/img/img.json`
    - `config/features/cat/user_list.json`
    - `config/features/cat/users/*.json`
    - `config/features/rua/rua.json`
    - `config/features/rua/rua_state.json`
- `config/port.txt`: deployment port file (kept at root for scripts)

## Operational Rules

1. Keep bot-core policy in `core`.
2. Put each feature under its own subdirectory in `features/<feature>/`.
3. Keep feature-owned runtime state in the same feature directory.
4. Avoid introducing new flat files directly under `config/features/`.

## Audit Tool

Run from project root:

```bash
python3 tools/audit_config_usage.py
```

It reports:
- unreferenced json files
- referenced-but-missing files
- configs without matching `_example.json`
- source to config usage mapping
