#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 2 by simulating one-piece copper completion."
print_pico_config pico2
publish "escape/pico2/copper_puzzle_complete" "manual copper test"
