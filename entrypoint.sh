#!/bin/bash
set -euo pipefail

cd /workspace

if [ "${SKIP_BUILD:-0}" != "1" ]; then
	./build.sh
fi

tail -f /dev/null
