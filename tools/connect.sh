#!/usr/bin/env bash
set -uo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
VOLUME="${VOLUME:-80%}"
TEST_SOUND="${TEST_SOUND:-${PROJECT_ROOT}/assets/audio/yeah-you-did-it.mp3}"

if command -v wpctl >/dev/null 2>&1; then
    wpctl set-volume @DEFAULT_AUDIO_SINK@ "${VOLUME}" || true
    wpctl set-mute @DEFAULT_AUDIO_SINK@ 0 || true
elif command -v amixer >/dev/null 2>&1; then
    amixer sset Master "${VOLUME}" unmute >/dev/null 2>&1 || true
fi

echo "Wired audio output ready. Plug the speaker into the Raspberry Pi 3.5mm jack."

if [ -f "${TEST_SOUND}" ]; then
    "${PROJECT_ROOT}/tools/play-audio.sh" "${TEST_SOUND}" || true
else
    echo "Test sound not found: ${TEST_SOUND}"
fi
