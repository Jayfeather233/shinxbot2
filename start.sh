#!/bin/bash
set -euo pipefail

mkdir -p log
nohup ./nohup.sh > ./log/nohup.out 2>&1 &