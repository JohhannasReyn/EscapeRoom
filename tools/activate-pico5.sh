#!/usr/bin/env bash
set -euo pipefail

mosquitto_pub -h "${MQTT_HOST:-localhost}" -t 'escape/cmd/pico5/enable_color_button_sequence' -m 'on'
