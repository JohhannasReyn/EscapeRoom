#!/usr/bin/env bash
set -euo pipefail

exec journalctl -u escape-room-controller.service -u escape-room-tv-dashboard.service -f
