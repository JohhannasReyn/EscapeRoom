#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 5 by simulating color button sequence completion."
print_pico_config pico5
publish "escape/pico5/color_sequence_complete" "manual color button test"
