#!/bin/bash
SCRIPT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" || exit ; pwd -P )
exec "$SCRIPT_DIR/../../tools/dev_tools/sender.sh" "$@"
