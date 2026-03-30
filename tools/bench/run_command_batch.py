#!/usr/bin/env python3
"""
Run batch command-matching benchmarks without bot backend or .so loading.
This is a script-level benchmark for local command-set iteration.
"""

import argparse
import json
import time
from pathlib import Path
from typing import List


def _norm_str_list(values) -> List[str]:
    return [str(x) for x in (values or [])]


def load_cases(profile: dict) -> list[str]:
    if "cases" in profile and isinstance(profile["cases"], list):
        return [str(x) for x in profile["cases"]]
    case_file = profile.get("cases_file")
    if case_file:
        p = Path(str(case_file))
        if p.exists():
            return [line.strip() for line in p.read_text(encoding="utf-8").splitlines() if line.strip()]
    return []


def run_profile(profile: dict) -> dict:
    name = str(profile.get("name", "unnamed"))
    exact = _norm_str_list(profile.get("exact", []))
    prefix = _norm_str_list(profile.get("prefix", []))
    loops = int(profile.get("loops", 50000))
    cases = load_cases(profile)

    if not cases:
        cases = ["*unknown", "help", "test"]

    matched = 0
    t0 = time.perf_counter_ns()
    for _ in range(loops):
        for cmd in cases:
            hit = (cmd in exact) or any(cmd.startswith(p) for p in prefix)
            if hit:
                matched += 1
    t1 = time.perf_counter_ns()

    total_calls = loops * len(cases)
    total_ns = t1 - t0
    ns_per_call = total_ns // total_calls if total_calls else 0
    return {
        "name": name,
        "cases": len(cases),
        "loops": loops,
        "total_calls": total_calls,
        "matched": matched,
        "total_ns": total_ns,
        "ns_per_call": ns_per_call,
    }


def run_scenarios(profile: dict) -> dict:
    name = str(profile.get("name", "unnamed"))
    exact = set(_norm_str_list(profile.get("exact", [])))
    prefix = set(_norm_str_list(profile.get("prefix", [])))
    scenarios = profile.get("scenarios", [])

    total_steps = 0
    passed_steps = 0
    t0 = time.perf_counter_ns()

    for sc in scenarios:
        steps = sc.get("steps", [])
        for step in steps:
            cmd = str(step.get("cmd", ""))
            expect_match = step.get("expect_match")
            matched = (cmd in exact) or any(cmd.startswith(p) for p in prefix)

            if expect_match is None or bool(expect_match) == matched:
                passed_steps += 1
            total_steps += 1

            for x in _norm_str_list(step.get("add_exact", [])):
                exact.add(x)
            for x in _norm_str_list(step.get("remove_exact", [])):
                exact.discard(x)
            for x in _norm_str_list(step.get("add_prefix", [])):
                prefix.add(x)
            for x in _norm_str_list(step.get("remove_prefix", [])):
                prefix.discard(x)

    t1 = time.perf_counter_ns()
    return {
        "name": name,
        "scenario_count": len(scenarios),
        "total_steps": total_steps,
        "passed_steps": passed_steps,
        "total_ns": t1 - t0,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Batch command matcher benchmark")
    parser.add_argument("--manifest", required=True, help="JSON manifest path")
    parser.add_argument("--out", help="Optional output json report path")
    args = parser.parse_args()

    manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    profiles = manifest.get("profiles", [])
    results = [run_profile(p) for p in profiles]
    scenario_results = [run_scenarios(p) for p in profiles if p.get("scenarios")]

    for r in results:
        print(
            f"[{r['name']}] cases={r['cases']} loops={r['loops']} "
            f"calls={r['total_calls']} matched={r['matched']} "
            f"ns_per_call={r['ns_per_call']}"
        )

    for s in scenario_results:
        print(
            f"[{s['name']}:scenario] scenarios={s['scenario_count']} "
            f"steps={s['passed_steps']}/{s['total_steps']} total_ns={s['total_ns']}"
        )

    if args.out:
        out_path = Path(args.out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(
            json.dumps({"results": results, "scenario_results": scenario_results}, indent=2),
            encoding="utf-8",
        )
        print(f"Report written: {out_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
