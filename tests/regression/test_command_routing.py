#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def must_contain(path: Path, needles):
    text = path.read_text(encoding="utf-8")
    missing = [n for n in needles if n not in text]
    if missing:
        print(f"[FAIL] {path}: missing markers -> {missing}")
        return False
    return True


def assert_auto92():
    path = ROOT / "src/functions/auto92/auto92.cpp"
    return must_contain(
        path,
        [
            "CMD_CLEAR_CACHE",
            "CMD_PRECOMPUTE_PREFIX",
            "cmd_match_exact(m,",
            "cmd_match_prefix(",
            "CMD_PREFIX_COMPAT",
        ],
    )


def assert_rua():
    path = ROOT / "src/functions/rua/rua.cpp"
    return must_contain(
        path,
        [
            "CMD_DRAW",
            "std::string(CMD_DRAW) + \" \"",
            "CMD_HELP",
            "CMD_RELOAD",
            "CMD_FAVOR_ZH",
        ],
    )


def assert_nonogram():
    path = ROOT / "src/functions/nonogram/nonogram.cpp"
    ok = must_contain(
        path,
        [
            "is_nonogram_command_message",
            "process_command",
            "\"help\"",
            "\"reload\"",
            "\"quit\"",
            "\"giveup\"",
            "\"online\"",
            "\"guess\"",
            "\"debug\"",
        ],
    )
    text = path.read_text(encoding="utf-8")
    if "bool nonogram::check" not in text:
        print(f"[FAIL] {path}: check() implementation not found")
        return False
    if not re.search(r"has_pending_guess\(conf\)", text):
        print(f"[FAIL] {path}: pending guess path is missing")
        ok = False
    if not re.search(r"has_pending_debug\(conf\)", text):
        print(f"[FAIL] {path}: pending debug path is missing")
        ok = False
    return ok


def main():
    checks = [
        ("auto92 routing", assert_auto92),
        ("rua routing", assert_rua),
        ("nonogram routing", assert_nonogram),
    ]

    failed = []
    for name, fn in checks:
        if fn():
            print(f"[OK] {name}")
        else:
            failed.append(name)

    if failed:
        print("\nRegression checks failed:")
        for name in failed:
            print(f"- {name}")
        return 1

    print("\nAll command routing regression checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
