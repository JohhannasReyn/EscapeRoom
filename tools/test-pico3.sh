#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 3 by simulating painting rotation completion."
print_pico_config pico3
publish "escape/pico3/painting_rotation_complete" "manual painting test"
