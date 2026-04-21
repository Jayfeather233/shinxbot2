#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

FUNCTION_DIR = ROOT / "src" / "functions"
EVENT_DIR = ROOT / "src" / "events"

REQ_FUNCTION_DECLS = ["process(", "check(", "help("]
REQ_FUNCTION_IMPL = ["DECLARE_FACTORY_FUNCTIONS("]
REQ_EVENT_DECLS = ["process(", "check("]
REQ_EVENT_IMPL = ["DECLARE_FACTORY_FUNCTIONS("]


def list_modules(base: Path) -> list[Path]:
    return sorted([p for p in base.iterdir() if p.is_dir() and not p.name.startswith(".")])


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def collect_module_text(module_dir: Path) -> str:
    parts: list[str] = []
    for p in sorted(module_dir.glob("*.h")) + sorted(module_dir.glob("*.hpp")) + sorted(module_dir.glob("*.cpp")):
        if p.name == "main.cpp":
            continue
        parts.append(read_text(p))
    return "\n".join(parts)


def check_function_module(module_dir: Path) -> list[str]:
    issues = []
    header_candidates = list(module_dir.glob("*.h")) + list(module_dir.glob("*.hpp"))
    cpp_candidates = [p for p in module_dir.glob("*.cpp") if p.name != "main.cpp"]
    if not header_candidates:
        return ["missing header file"]
    if not cpp_candidates:
        return ["missing cpp file"]

    module_text = collect_module_text(module_dir)

    if "class" not in module_text or "public processable" not in module_text:
        issues.append("header does not declare a processable subclass")

    for marker in REQ_FUNCTION_DECLS:
        if marker not in module_text:
            issues.append(f"missing function contract marker {marker}")

    for marker in REQ_FUNCTION_IMPL:
        if marker not in module_text:
            issues.append(f"missing marker {marker}")

    return issues


def check_event_module(module_dir: Path) -> list[str]:
    issues = []
    header_candidates = list(module_dir.glob("*.h")) + list(module_dir.glob("*.hpp"))
    cpp_candidates = [p for p in module_dir.glob("*.cpp") if p.name != "main.cpp"]
    if not header_candidates:
        return ["missing header file"]
    if not cpp_candidates:
        return ["missing cpp file"]

    module_text = collect_module_text(module_dir)

    if "class" not in module_text or "public eventprocess" not in module_text:
        issues.append("header does not declare an eventprocess subclass")

    for marker in REQ_EVENT_DECLS:
        if marker not in module_text:
            issues.append(f"missing event contract marker {marker}")

    for marker in REQ_EVENT_IMPL:
        if marker not in module_text:
            issues.append(f"missing marker {marker}")

    return issues


def main() -> int:
    failures = []

    for module_dir in list_modules(FUNCTION_DIR):
        if module_dir.name == "build":
            continue
        issues = check_function_module(module_dir)
        if issues:
            failures.append((f"function:{module_dir.name}", issues))

    for module_dir in list_modules(EVENT_DIR):
        if module_dir.name == "build":
            continue
        issues = check_event_module(module_dir)
        if issues:
            failures.append((f"event:{module_dir.name}", issues))

    if failures:
        for name, issues in failures:
            print(f"[FAIL] {name}")
            for issue in issues:
                print(f"  - {issue}")
        return 1

    print("All function/event modules passed the contract scan.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
