#!/usr/bin/env bash
set -euo pipefail

sudo systemctl stop escape-room-tv-dashboard.service 2>/dev/null || true
sudo systemctl stop escape-room-controller.service
tools/room-status.sh
