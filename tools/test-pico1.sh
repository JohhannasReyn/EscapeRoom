#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 1 by simulating motion sensor trigger."
print_pico_config pico1
publish "escape/pico1/cubby_approach_detected" "manual motion test"
