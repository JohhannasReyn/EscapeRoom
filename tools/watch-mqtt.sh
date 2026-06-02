#!/usr/bin/env bash
set -euo pipefail

exec mosquitto_sub -h "${MQTT_HOST:-localhost}" -v -t "${MQTT_TOPIC:-escape/#}"
