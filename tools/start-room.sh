#!/usr/bin/env bash
set -euo pipefail

sudo systemctl start escape-room-controller.service
tools/room-status.sh
