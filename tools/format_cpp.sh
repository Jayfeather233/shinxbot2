#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
cd "$ROOT_DIR"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "Error: clang-format not found in PATH" >&2
  exit 1
fi

# Only format C/C++ sources and headers. Do not touch shell scripts.
mapfile -d '' FILES < <(
  find src tools \
    -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
    -not -path '*/build/*' \
    -not -path '*/lib/*' \
    -print0
)

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No C/C++ files found to format."
  exit 0
fi

clang-format -i "${FILES[@]}"
echo "Formatted ${#FILES[@]} files with clang-format."
