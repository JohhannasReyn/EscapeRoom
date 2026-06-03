#!/usr/bin/env bash
set -euo pipefail

sudo systemctl stop escape-room-tv-dashboard.service
sudo systemctl stop escape-room-controller.service
tools/room-status.sh
