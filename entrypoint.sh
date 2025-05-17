#!/bin/bash

cd /workspace
./build.sh
./build_features.sh

tail -f /dev/null
