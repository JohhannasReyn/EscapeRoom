#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

need_cmd mosquitto_sub
need_cmd timeout

PRESS_TIMEOUT="${PRESS_TIMEOUT:-20}"
RESET_PRESS_TIMEOUT="${RESET_PRESS_TIMEOUT:-12}"
LOG_DIR="${FIRE_PANEL_LOG_DIR:-${PROJECT_ROOT}}"
LOG_FILE="${1:-${LOG_DIR}/fire-panel-button-order-$(date +%Y%m%d-%H%M%S).txt}"

mkdir -p "$(dirname "${LOG_FILE}")"

cat >"${LOG_FILE}" <<EOF
Fire panel button order capture
Started: $(date -Is)
MQTT host: ${MQTT_HOST}

Ask the student to press each physical button when prompted.
The script records the MQTT event that Pico 7 / Pico 6 fire panel actually publishes.

EOF

BUTTONS=(
    "escape/fire/status|STATUS|Press the button labeled STATUS|${PRESS_TIMEOUT}"
    "escape/fire/film-on|FILM-ON|Press the button labeled FILM-ON|${PRESS_TIMEOUT}"
    "escape/fire/film-off|FILM-OFF|Press the button labeled FILM-OFF|${PRESS_TIMEOUT}"
    "escape/fire/sound-look|SOUND-LOOK|Press the button labeled SOUND-LOOK|${PRESS_TIMEOUT}"
    "escape/fire/sound-crash|SOUND-CRASH|Press the button labeled SOUND-CRASH|${PRESS_TIMEOUT}"
    "escape/fire/sound-fail|SOUND-FAIL|Press the button labeled SOUND-FAIL|${PRESS_TIMEOUT}"
    "escape/fire/sound-pass|SOUND-PASS|Press the button labeled SOUND-PASS|${PRESS_TIMEOUT}"
    "escape/fire/sound-bake|SOUND-BAKE|Press the button labeled SOUND-BAKE|${PRESS_TIMEOUT}"
    "escape/fire/unlock|UNLOCK|Press the button labeled UNLOCK|${PRESS_TIMEOUT}"
    "escape/fire/reset-all|RESET-ALL|Press the button labeled RESET-ALL and hold it for 5 seconds|${RESET_PRESS_TIMEOUT}"
)

echo "Fire panel button order capture"
echo "Output file: ${LOG_FILE}"
echo
echo "Make sure the room controller or MQTT broker is running, then follow the prompts."
echo "For RESET-ALL, hold the button for 5 seconds."
echo

step=1
for entry in "${BUTTONS[@]}"; do
    IFS='|' read -r expected_topic label prompt timeout_sec <<<"${entry}"

    echo "Step ${step}: ${prompt}"
    echo "Waiting up to ${timeout_sec} seconds for ${MQTT_TOPIC_ROOT}/fire/# ..."

    line="$(timeout "${timeout_sec}" mosquitto_sub -h "${MQTT_HOST}" -C 1 -v -t "${MQTT_TOPIC_ROOT}/fire/#" 2>/dev/null || true)"
    timestamp="$(date -Is)"

    observed_topic="(no event)"
    observed_payload=""
    result="NO EVENT"

    if [ -n "${line}" ]; then
        observed_topic="${line%% *}"
        if [ "${observed_topic}" != "${line}" ]; then
            observed_payload="${line#* }"
        fi

        if [ "${observed_topic}" = "${expected_topic}" ]; then
            result="OK"
        else
            result="MISMATCH"
        fi
    fi

    {
        echo "Step ${step}: ${label}"
        echo "  Expected: ${expected_topic}"
        echo "  Observed: ${observed_topic}"
        echo "  Payload: ${observed_payload}"
        echo "  Result: ${result}"
        echo "  Time: ${timestamp}"
        echo
    } >>"${LOG_FILE}"

    echo "  Observed: ${observed_topic} ${observed_payload}"
    echo "  Result: ${result}"
    echo

    step=$((step + 1))
done

cat >>"${LOG_FILE}" <<EOF
Finished: $(date -Is)

If any result is MISMATCH or NO EVENT, send this file so we can confirm the
current order or remap the Pico fire-panel buttons in firmware.
EOF

echo "Capture complete."
echo "Please send this file:"
echo "  ${LOG_FILE}"
