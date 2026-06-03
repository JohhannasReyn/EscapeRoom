#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"
RUN_USER="${RUN_USER:-admin}"
SERVICE_PATH="/etc/systemd/system/escape-room-controller.service"
TV_SERVICE_PATH="/etc/systemd/system/escape-room-tv-dashboard.service"

echo "Installing escape room autostart for project root: ${PROJECT_ROOT}"

sudo chmod +x "${PROJECT_ROOT}"/tools/*.sh

if nmcli connection show escape-room-hotspot >/dev/null 2>&1; then
    sudo nmcli connection modify escape-room-hotspot connection.autoconnect yes
else
    echo "escape-room-hotspot does not exist yet; creating it now."
    bash "${PROJECT_ROOT}/tools/start-wifi.sh"
fi

cat <<SERVICE | sudo tee "${SERVICE_PATH}" >/dev/null
[Unit]
Description=Escape Room Raspberry Pi Controller
After=network-online.target bluetooth.service mosquitto.service
Wants=network-online.target bluetooth.service
Requires=mosquitto.service

[Service]
Type=simple
User=${RUN_USER}
WorkingDirectory=${PROJECT_ROOT}
Environment=PROJECT_ROOT=${PROJECT_ROOT}
Environment=PLATFORMIO_VENV=/home/${RUN_USER}/.platformio-venv
ExecStartPre=-${PROJECT_ROOT}/tools/connect-bluetooth.sh
ExecStart=${PROJECT_ROOT}/tools/monitor-puzzles.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
SERVICE

cat <<SERVICE | sudo tee "${TV_SERVICE_PATH}" >/dev/null
[Unit]
Description=Escape Room HDMI TV Dashboard
After=mosquitto.service escape-room-controller.service
Requires=mosquitto.service

[Service]
Type=simple
User=${RUN_USER}
WorkingDirectory=${PROJECT_ROOT}
Environment=TERM=linux
Environment=TV_CONSOLE=/dev/tty1
Environment=TV_FONT=Lat2-Terminus32x16.psf.gz
ExecStartPre=+${PROJECT_ROOT}/tools/set-tv-font.sh
ExecStart=${PROJECT_ROOT}/tools/tv-dashboard.sh
StandardInput=tty
StandardOutput=tty
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
SERVICE

sudo systemctl daemon-reload
sudo systemctl enable escape-room-controller.service
sudo systemctl enable escape-room-tv-dashboard.service

echo
echo "Autostart installed."
echo "Start now with:"
echo "  tools/start-room.sh"
echo
echo "Check status with:"
echo "  tools/room-status.sh"
echo
echo "Watch logs with:"
echo "  tools/room-logs.sh"
