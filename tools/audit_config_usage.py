#!/usr/bin/env python3
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
CFG = ROOT / "config"
TOOLS = ROOT / "tools"

PATTERN = re.compile(r'(?:\./|\.\./|\.\./\.\./)?config/([A-Za-z0-9_./-]+\.json)')

SOURCE_SUFFIX = {".cpp", ".h", ".hpp", ".c", ".cc", ".py", ".sh"}


def iter_source_files():
    for base in (SRC, TOOLS):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.is_file() and path.suffix in SOURCE_SUFFIX:
                yield path


def collect_referenced_configs():
    refs = {}
    for path in iter_source_files():
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue
        found = PATTERN.findall(text)
        if found:
            refs[str(path.relative_to(ROOT))] = sorted(set(found))
    return refs


def flatten_refs(refs):
    used = set()
    for items in refs.values():
        used.update(items)
    return used


def main():
    refs = collect_referenced_configs()
    used = flatten_refs(refs)

    existing = sorted(
        [str(p.relative_to(CFG)) for p in CFG.rglob("*.json") if p.is_file()]
    )

    unreferenced = [x for x in existing if x not in used and not x.endswith("_example.json")]
    referenced_missing = [x for x in sorted(used) if not (CFG / x).exists()]

    example_map = {x for x in existing if x.endswith("_example.json")}
    base_missing_example = []
    for x in existing:
        if x.endswith("_example.json"):
            continue
        stem = x[:-5]
        if stem + "_example.json" not in example_map:
            base_missing_example.append(x)

    report = {
        "summary": {
            "source_files_with_config_calls": len(refs),
            "referenced_config_files": len(used),
            "existing_json_files": len(existing),
            "unreferenced_json_files": len(unreferenced),
            "referenced_but_missing": len(referenced_missing),
            "missing_example_pair": len(base_missing_example),
        },
        "unreferenced_json_files": unreferenced,
        "referenced_but_missing": referenced_missing,
        "missing_example_pair": base_missing_example,
        "references_by_source": refs,
    }

    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
