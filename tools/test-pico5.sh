#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 5 by enabling the color button sequence."
print_pico_config pico5
publish "escape/cmd/pico5/enable_color_button_sequence" "on"
