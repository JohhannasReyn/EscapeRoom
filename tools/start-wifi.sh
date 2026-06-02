#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"

cd "${PROJECT_ROOT}"
bash tools/setup-pi-hotspot.sh

echo
echo "Active connections:"
nmcli connection show --active

echo
echo "wlan0 address:"
ip -4 addr show wlan0 || true

echo
echo "Connected WiFi stations:"
sudo iw dev wlan0 station dump || true
