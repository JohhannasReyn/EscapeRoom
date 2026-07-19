#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"

cd "${PROJECT_ROOT}"

echo "Escape room audio test"
echo
echo "1. Direct Pi speaker test using the same wired-output path as the controller."
tools/play-audio.sh assets/audio/yeah-you-did-it.mp3
tools/play-audio.sh assets/audio/check-the-oven.wav

echo
echo "2. Controller/MQTT fire-command test."
if ! command -v mosquitto_pub >/dev/null 2>&1; then
    echo "Missing mosquitto_pub. Install mosquitto-clients or run tools/rebase.sh/setup-room first."
    exit 1
fi

fire/sound-pass
sleep 1
fire/sound-look

echo
echo "If direct audio worked but the fire commands did not, the controller service is not receiving or processing MQTT."
echo "Check it with:"
echo "  tools/room-status.sh"
echo "  tools/room-logs.sh"
echo
echo "In the logs, each fire command should show 'MQTT message received', then 'Queued audio', then 'Playing queued audio'."
