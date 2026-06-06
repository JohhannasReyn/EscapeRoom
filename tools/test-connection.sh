#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

need_cmd mosquitto_pub
need_cmd mosquitto_sub
need_cmd timeout

echo "MQTT broker: ${MQTT_HOST}"
echo "Pi hostname: $(hostname).local"
echo "Pi addresses: $(hostname -I 2>/dev/null || true)"
echo
wait_for_topics_once "${PICO_STATUS_TIMEOUT}"
