#!/usr/bin/env python3
"""
Generate a local check() benchmark source for one function .so module.
This benchmark does not send messages to backend and only calls check().
"""

import argparse
import json
from pathlib import Path

CPP_TEMPLATE = r'''#include "bot.h"
#include "dynamic_lib.hpp"
#include "processable.h"

#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <vector>

class DummyBot : public bot {
public:
    DummyBot() : bot(0, 0, "") {}

    void run() override {}
    void cq_send_all_op(const std::string &message) override { (void)message; }
    void input_process(const std::string &input) override { (void)input; }
};

int main()
{
    const std::string so_path = "__SO_PATH__";
    auto loaded = load_function<processable>(so_path);
    processable *func = loaded.first;
    void *handle = loaded.second;
    if (!func || !handle) {
        std::cerr << "Failed to load module: " << so_path << "\n";
        return 1;
    }

    using destroy_t = void (*)(processable *);
    destroy_t destroy = reinterpret_cast<destroy_t>(dlsym(handle, "destroy_t"));
    if (!destroy) {
        std::cerr << "Failed to locate destroy_t in module\n";
        dlclose(handle);
        return 1;
    }

    DummyBot bot;
    msg_meta conf("group", 10001, 10002, 10003, &bot);

    const std::vector<std::string> cases = {
__CASES__
    };

    constexpr int kOuterLoops = __LOOPS__;
    size_t matched = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kOuterLoops; ++i) {
        for (const auto &cmd : cases) {
            if (func->check(cmd, conf)) {
                matched++;
            }
        }
    }
    auto t1 = std::chrono::steady_clock::now();

    const auto total_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    const size_t total_calls = static_cast<size_t>(kOuterLoops) * cases.size();

    std::cout << "module=" << so_path << "\n";
    std::cout << "cases=" << cases.size() << " total_calls=" << total_calls
              << " matched=" << matched << "\n";
    std::cout << "total_ns=" << total_ns << " ns_per_call="
              << (total_calls ? (total_ns / static_cast<long long>(total_calls)) : 0)
              << "\n";

    destroy(func);
    dlclose(handle);
    return 0;
}
'''


def generate_one(so: str, cases: list[str], loops: int, out: str) -> None:
    cases_cpp = "\n".join(f'        "{c}",' for c in cases)
    out_path = Path(out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    content = CPP_TEMPLATE
    content = content.replace("__SO_PATH__", so)
    content = content.replace("__CASES__", cases_cpp)
    content = content.replace("__LOOPS__", str(loops))

    out_path.write_text(content, encoding="utf-8")
    print(f"Generated: {out_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate function check benchmark source")
    parser.add_argument("--so", help="Path to target .so, e.g. lib/functions/libguessmap.so")
    parser.add_argument("--case", action="append", default=[], help="Benchmark input command; repeatable")
    parser.add_argument("--loops", type=int, default=50000, help="Outer loop count")
    parser.add_argument("--out", default="tests/function_bench/generated_check_bench.cpp", help="Output cpp path")
    parser.add_argument("--batch-manifest", help="JSON manifest for batch generation")
    args = parser.parse_args()

    if args.batch_manifest:
        manifest = json.loads(Path(args.batch_manifest).read_text(encoding="utf-8"))
        jobs = manifest.get("jobs", [])
        for i, job in enumerate(jobs):
            so = str(job.get("so", "")).strip()
            if not so:
                continue
            name = str(job.get("name", f"job_{i}"))
            cases = job.get("cases") or ["*unknown", "*help", "test"]
            loops = int(job.get("loops", args.loops))
            out = job.get(
                "out",
                f"tests/function_bench/generated/{name}_check_bench.cpp",
            )
            generate_one(so, cases, loops, out)
        return 0

    if not args.so:
        raise SystemExit("--so is required when --batch-manifest is not provided")

    cases = args.case or ["*unknown", "*help", "test"]
    generate_one(args.so, cases, args.loops, args.out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
