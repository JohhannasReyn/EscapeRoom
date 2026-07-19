#!/usr/bin/env bash
set -euo pipefail

LINES="${1:-200}"
SERVICE="${ESCAPE_ROOM_SERVICE:-escape-room-controller.service}"

echo "Recent fail-safe activity from ${SERVICE}:"
echo

if ! command -v journalctl >/dev/null 2>&1; then
    echo "journalctl is not available here. Use tools/room-logs.sh on the Raspberry Pi and search for FAIL_SAFE."
    exit 0
fi

if ! journalctl -u "${SERVICE}" -n "${LINES}" --no-pager 2>/dev/null | grep -i "FAIL_SAFE"; then
    echo "No FAIL_SAFE entries found in the last ${LINES} log lines."
fi

echo
echo "Live controller logs:"
echo "  tools/room-logs.sh"
