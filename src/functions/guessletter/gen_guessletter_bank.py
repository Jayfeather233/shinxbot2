#!/usr/bin/env python3
import json
import re
from pathlib import Path

try:
    from pypinyin import Style, lazy_pinyin
    HAS_PYPINYIN = True
except Exception:
    HAS_PYPINYIN = False

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "config/features/guessletter/letterbank.json"
SOURCE = OUT


def norm_key(s: str) -> str:
    return "".join(ch.lower() for ch in s if not ch.isspace())


def pinyin_initials_key(cn_name: str) -> str:
    cn_name = cn_name.strip()
    if not cn_name:
        return ""

    # Numeric-only names should keep original digits, e.g. "74" -> "74".
    if re.fullmatch(r"\d+", cn_name):
        return cn_name

    out = []
    i = 0
    while i < len(cn_name):
        ch = cn_name[i]
        if ch.isspace():
            i += 1
            continue

        # Keep contiguous ASCII text unchanged in pinyin mode.
        if ch.isascii() and ch.isalnum():
            j = i + 1
            while j < len(cn_name) and cn_name[j].isascii() and cn_name[j].isalnum():
                j += 1
            out.append(cn_name[i:j].lower())
            i = j
            continue

        if HAS_PYPINYIN:
            token = lazy_pinyin(ch, style=Style.FIRST_LETTER, errors="ignore")
            if token and token[0]:
                c = token[0][0].lower()
                if c.isalnum():
                    out.append(c)
        i += 1

    if out:
        return "".join(out)

    # Fallback when pypinyin is unavailable: keep ASCII letters/digits only.
    return norm_key(cn_name)


def load_entries_json(path: Path):
    root = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(root, list):
        return [x for x in root if isinstance(x, dict)]
    if isinstance(root, dict) and isinstance(root.get("entries"), list):
        return [x for x in root["entries"] if isinstance(x, dict)]
    return []


def normalize_entry(e: dict):
    scope = trim_text(e.get("scope", ""))
    cn_name = trim_text(e.get("cn_name", ""))
    en_name = trim_text(e.get("en_name", ""))
    if not scope or (not cn_name and not en_name):
        return None

    pinyin_key = norm_key(str(e.get("pinyin_key", "")))
    english_key = norm_key(str(e.get("english_key", "")))

    if not pinyin_key and cn_name:
        pinyin_key = pinyin_initials_key(cn_name)
    if not pinyin_key and en_name:
        pinyin_key = norm_key(en_name)
    if not english_key and en_name:
        english_key = norm_key(en_name)

    aliases = []
    if isinstance(e.get("aliases"), list):
        for a in e["aliases"]:
            s = trim_text(a)
            if s:
                aliases.append(s)

    # Deduplicate aliases and remove self aliases.
    seen = set()
    filtered_aliases = []
    self_keys = set()
    if cn_name:
        self_keys.add(norm_key(cn_name))
    if en_name:
        self_keys.add(norm_key(en_name))
    for a in aliases:
        k = norm_key(a)
        if not k or k in self_keys or k in seen:
            continue
        seen.add(k)
        filtered_aliases.append(a)

    out = {
        "scope": scope,
        "cn_name": cn_name,
        "en_name": en_name,
        "pinyin_key": pinyin_key,
        "english_key": english_key,
    }
    if filtered_aliases:
        out["aliases"] = filtered_aliases
    return out


def trim_text(v):
    return str(v).strip()


def entry_key(e: dict):
    scope = norm_key(e.get("scope", ""))
    cn = norm_key(e.get("cn_name", ""))
    en = norm_key(e.get("en_name", ""))
    if cn:
        return f"{scope}|cn|{cn}"
    return f"{scope}|en|{en}"


def alias_keys(e: dict):
    scope = norm_key(e.get("scope", ""))
    out = []
    cn = norm_key(e.get("cn_name", ""))
    en = norm_key(e.get("en_name", ""))
    if cn:
        out.append(f"{scope}|cn|{cn}")
    if en:
        out.append(f"{scope}|en|{en}")
    return out


def merge_entries(old_e: dict, new_e: dict):
    merged = dict(old_e)
    # Newer source line is authoritative for explicit name edits.
    for key in ["cn_name", "en_name", "pinyin_key", "english_key"]:
        if trim_text(new_e.get(key, "")):
            merged[key] = new_e[key]

    # Recompute keys after merge to keep consistency.
    if not trim_text(merged.get("pinyin_key", "")) and trim_text(
        merged.get("cn_name", "")
    ):
        merged["pinyin_key"] = pinyin_initials_key(merged["cn_name"])
    if not trim_text(merged.get("pinyin_key", "")) and trim_text(
        merged.get("en_name", "")
    ):
        merged["pinyin_key"] = norm_key(merged["en_name"])
    if not trim_text(merged.get("english_key", "")) and trim_text(
        merged.get("en_name", "")
    ):
        merged["english_key"] = norm_key(merged["en_name"])

    # Union aliases from both entries.
    alias_src = []
    if isinstance(old_e.get("aliases"), list):
        alias_src.extend(old_e.get("aliases"))
    if isinstance(new_e.get("aliases"), list):
        alias_src.extend(new_e.get("aliases"))

    merged_aliases = []
    seen = set()
    self_keys = set()
    if trim_text(merged.get("cn_name", "")):
        self_keys.add(norm_key(merged["cn_name"]))
    if trim_text(merged.get("en_name", "")):
        self_keys.add(norm_key(merged["en_name"]))
    for a in alias_src:
        s = trim_text(a)
        k = norm_key(s)
        if not s or not k or k in self_keys or k in seen:
            continue
        seen.add(k)
        merged_aliases.append(s)

    if merged_aliases:
        merged["aliases"] = merged_aliases
    elif "aliases" in merged:
        del merged["aliases"]

    return merged


def dedupe_entries(new_entries):
    out = []
    alias_to_idx = {}
    merged_count = 0
    for raw in new_entries:
        e = normalize_entry(raw)
        if e is None:
            continue

        aliases = alias_keys(e)
        hit = None
        for a in aliases:
            if a in alias_to_idx:
                hit = alias_to_idx[a]
                break

        if hit is None:
            idx = len(out)
            out.append(e)
            for a in aliases:
                alias_to_idx[a] = idx
            continue

        out[hit] = merge_entries(out[hit], e)
        merged_count += 1
        for a in alias_keys(out[hit]):
            alias_to_idx[a] = hit

    # Stable deterministic order for cleaner diffs.
    out.sort(
        key=lambda x: (
            norm_key(x.get("scope", "")),
            norm_key(x.get("cn_name", "")),
            norm_key(x.get("en_name", "")),
        )
    )
    return out, merged_count


def main():
    if not SOURCE.exists():
        raise SystemExit(f"Missing source file: {SOURCE}")

    entries, merged_count = dedupe_entries(load_entries_json(SOURCE))

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
    print(f"Merged duplicates: {merged_count}")
    scope_count = {}
    for e in entries:
        scope = e.get("scope", "")
        scope_count[scope] = scope_count.get(scope, 0) + 1
    print("Scopes:", json.dumps(scope_count, ensure_ascii=False, sort_keys=True))
    if not HAS_PYPINYIN:
        print("Warning: pypinyin not installed; Chinese initials may be incomplete.")


if __name__ == "__main__":
    main()
