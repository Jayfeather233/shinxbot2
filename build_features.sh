#!/bin/bash
set -euo pipefail

pushd ./src/functions >/dev/null
python3 generate_cmake.py
./make_all.sh
popd >/dev/null

pushd ./src/events >/dev/null
python3 generate_cmake.py
./make_all.sh
popd >/dev/null