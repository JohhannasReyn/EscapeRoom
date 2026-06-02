#!/usr/bin/env bash
set -euo pipefail

HOTSPOT_NAME="${HOTSPOT_NAME:-escape-room-hotspot}"
HOTSPOT_SSID="${HOTSPOT_SSID:-EscapeRoom}"
HOTSPOT_PASSWORD="${HOTSPOT_PASSWORD:-BakeAt350}"
HOTSPOT_INTERFACE="${HOTSPOT_INTERFACE:-wlan0}"
HOTSPOT_ADDRESS="${HOTSPOT_ADDRESS:-10.42.0.1/24}"

if ! command -v nmcli >/dev/null 2>&1; then
    echo "nmcli was not found. Install NetworkManager first:"
    echo "  sudo apt update"
    echo "  sudo apt install -y network-manager"
    exit 1
fi

if [ "${#HOTSPOT_PASSWORD}" -lt 8 ]; then
    echo "HOTSPOT_PASSWORD must be at least 8 characters for WPA-PSK."
    exit 1
fi

echo "Creating Raspberry Pi escape room hotspot:"
echo "  connection: ${HOTSPOT_NAME}"
echo "  ssid:       ${HOTSPOT_SSID}"
echo "  interface:  ${HOTSPOT_INTERFACE}"
echo "  address:    ${HOTSPOT_ADDRESS}"

if nmcli connection show "${HOTSPOT_NAME}" >/dev/null 2>&1; then
    sudo nmcli connection delete "${HOTSPOT_NAME}"
fi

sudo nmcli connection add \
    type wifi \
    ifname "${HOTSPOT_INTERFACE}" \
    con-name "${HOTSPOT_NAME}" \
    autoconnect yes \
    ssid "${HOTSPOT_SSID}"

sudo nmcli connection modify "${HOTSPOT_NAME}" \
    802-11-wireless.mode ap \
    802-11-wireless.band bg \
    ipv4.method shared \
    ipv4.addresses "${HOTSPOT_ADDRESS}" \
    wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "${HOTSPOT_PASSWORD}"

sudo nmcli connection up "${HOTSPOT_NAME}"

echo
echo "Hotspot is active."
echo "Configure Pico W firmware with:"
echo "  WIFI_SSID=${HOTSPOT_SSID}"
echo "  WIFI_PASS=${HOTSPOT_PASSWORD}"
echo "  MQTT_BROKER=${HOTSPOT_ADDRESS%/*}"
