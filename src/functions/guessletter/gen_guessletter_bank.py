#!/usr/bin/env python3
import json
import re
from pathlib import Path

try:
    from pypinyin import Style, lazy_pinyin
    HAS_PYPINYIN = True
except Exception:
    HAS_PYPINYIN = False

ROOT = Path(__file__).resolve().parents[1]
MAPS = ROOT / "config/guessmap/maps.json"
OUT = ROOT / "config/guessletter/letterbank.json"


def norm_key(s: str) -> str:
    return "".join(ch.lower() for ch in s if not ch.isspace())


def pinyin_initials_key(cn_name: str) -> str:
    cn_name = cn_name.strip()
    if not cn_name:
        return ""

    # Numeric-only names should keep original digits, e.g. "74" -> "74".
    if re.fullmatch(r"\d+", cn_name):
        return cn_name

    if HAS_PYPINYIN:
        parts = lazy_pinyin(cn_name, style=Style.FIRST_LETTER, errors="keep")
        out = []
        for token in parts:
            for ch in token:
                if ch.isalnum():
                    out.append(ch.lower())
                    break
        return "".join(out)

    # Fallback when pypinyin is unavailable: keep ASCII letters/digits only.
    return norm_key(cn_name)


def split_cn_en(answer: str):
    answer = answer.strip()
    if not answer:
        return "", ""
    m = re.search(r"[A-Za-z0-9]", answer)
    if not m:
        return answer, ""
    idx = m.start()
    cn = answer[:idx].strip()
    en = answer[idx:].strip()
    return cn, en


def first_scope_segment(file_path: str) -> str:
    p = (file_path or "").replace("\\", "/").strip("/")
    if not p:
        return ""
    return p.split("/", 1)[0].strip()


def parse_entry(item: dict):
    answer = str(item.get("answer", "")).strip()
    cn_name, en_name = split_cn_en(answer)
    file_path = str(item.get("file_path", "")).strip()

    cn_l = cn_name.lower()
    en_l = en_name.lower()
    fp_l = file_path.lower()
    if "大厅" in cn_name or "lobby" in en_l or "lobby" in fp_l:
        return None

    pinyin_key = norm_key(str(item.get("pinyin_initials") or item.get("cn_initials") or ""))
    if not pinyin_key and cn_name:
        pinyin_key = pinyin_initials_key(cn_name)
    english_key = norm_key(str(item.get("english_key") or item.get("en_full") or item.get("en") or en_name))

    if not cn_name and not en_name:
        return None

    return {
        "scope": first_scope_segment(file_path),
        "cn_name": cn_name,
        "en_name": en_name,
        "pinyin_key": pinyin_key,
        "english_key": english_key,
    }


def main():
    maps = json.loads(MAPS.read_text(encoding="utf-8"))
    if not isinstance(maps, list):
        raise SystemExit("maps.json must be a JSON array")

    entries = []
    for item in maps:
        if not isinstance(item, dict):
            continue
        e = parse_entry(item)
        if e is not None:
            entries.append(e)

    OUT.write_text(
        json.dumps({"entries": entries}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    total = len(entries)
    cn_ready = sum(1 for e in entries if e.get("cn_name") and e.get("pinyin_key"))
    en_ready = sum(1 for e in entries if e.get("en_name") and e.get("english_key"))
    print(f"Generated {total} entries to {OUT}")
    print(f"CN playable (has pinyin key): {cn_ready}")
    print(f"EN playable (has english key): {en_ready}")
    if not HAS_PYPINYIN:
        print("Warning: pypinyin not installed; Chinese initials may be incomplete.")


if __name__ == "__main__":
    main()
