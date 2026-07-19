#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
AUDIO_DEVICE="${ESCAPE_AUDIO_DEVICE:-plughw:CARD=Headphones,DEV=0}"

if [ "$#" -ne 1 ]; then
    echo "Usage: tools/play-audio.sh assets/audio/sound-file"
    exit 1
fi

audio_file="$1"

if [ ! -f "${audio_file}" ] && [ -f "${PROJECT_ROOT}/assets/audio/${audio_file}" ]; then
    audio_file="${PROJECT_ROOT}/assets/audio/${audio_file}"
fi

if [ ! -f "${audio_file}" ]; then
    echo "Audio file not found: ${audio_file}"
    exit 1
fi

missing=0
for cmd in ffmpeg aplay timeout; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        echo "Missing required audio command: ${cmd}"
        missing=1
    fi
done

if [ "${missing}" -ne 0 ]; then
    echo "Install the Pi audio tools with:"
    echo "  sudo apt install -y ffmpeg alsa-utils coreutils"
    exit 1
fi

echo "Playing ${audio_file}"
echo "Audio device: ${AUDIO_DEVICE}"
timeout --kill-after=2s 15s bash -o pipefail -c 'ffmpeg -hide_banner -loglevel error -nostdin -i "$1" -f wav - | aplay -q -D "$2" -' _ "${audio_file}" "${AUDIO_DEVICE}"
