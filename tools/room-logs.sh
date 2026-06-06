#!/usr/bin/env bash
set -euo pipefail

exec journalctl -u escape-room-controller.service -f
