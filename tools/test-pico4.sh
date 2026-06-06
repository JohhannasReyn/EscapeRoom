#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 4 by revealing smart film and enabling oven knob."
print_pico_config pico4
publish "escape/cmd/pico4/reveal_smart_film" "on"
sleep 2
publish "escape/cmd/pico4/enable_oven_knob" "on"
