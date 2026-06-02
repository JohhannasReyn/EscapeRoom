#!/usr/bin/env bash
set -euo pipefail

mosquitto_pub -h "${MQTT_HOST:-localhost}" -t 'escape/cmd/pico2/enable_copper_puzzle' -m 'on'
