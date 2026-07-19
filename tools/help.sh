#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CURRENT_DIR="$(pwd -P)"

if [ "${CURRENT_DIR}" != "${PROJECT_ROOT}" ]; then
    cat <<EOF
You are not in the escape-room project folder.
Most commands below should be run from there first:
  cd escape-room

If that does not work, use:
  cd ${PROJECT_ROOT}

EOF
fi

cat <<'EOF'
Escape room quick help

Core commands:
| Task | Command | Notes |
| --- | --- | --- |
| Update from repo | tools/rebase.sh | Pulls the latest code, refreshes permissions, installs help, and prints which Picos need to be flashed |
| Apply Pi setup | tools/setup-room.sh | Run from inside escape-room; do not run it with source |
| Start controller | tools/start.sh | Runs the room controller in the terminal; Pico wiring reports print here |
| Show this guide | help or tools/help.sh | help works from any SSH directory after rebase installs it |

Pico flashing:
| Pico | Command |
| --- | --- |
| Copper puzzle | tools/flash-pico.sh pico2 |
| Painting rotation | tools/flash-pico.sh pico3 |
| Smart film / oven / lock | tools/flash-pico.sh pico4 |
| Color buttons | tools/flash-pico.sh pico5 |
| Fire panel | tools/flash-pico.sh pico7 |
| All active Picos | tools/flash-pico.sh all |

For each Pico: unplug it, hold BOOTSEL, plug it into USB, release BOOTSEL, then
press Enter when the flash script asks.

Checks and logs:
| Need | Command | Notes |
| --- | --- | --- |
| Pico connectivity | tools/test-connection.sh | Requests status and wiring reports |
| Full walkthrough | tools/test-all.sh | Simulates the room sequence with MQTT |
| Copper puzzle | tools/test-pico2.sh | Simulates puzzle-piece placement |
| Painting rotation | tools/test-pico3.sh | Simulates picture rotation |
| Smart film / oven / lock | tools/test-pico4.sh | Reveals film, watches oven telemetry, simulates unlock |
| Color buttons | tools/test-pico5.sh | Tests wrong-code sounds and success flow |
| Speaker path | tools/connect.sh | Direct wired speaker check |
| Audio pipeline | tools/test-audio.sh | Direct audio plus controller/MQTT sound tests |
| Volume | tools/set-volume.sh -v 50 | Also available: tools/volume-up.sh and tools/volume-down.sh |
| Service status | tools/room-status.sh | Shows whether the room service is running |
| Room logs | tools/room-logs.sh | Watches controller service logs |
| Fail-safe log | tools/failsafe-status.sh | Shows recent FAIL_SAFE retries, backups, and failures |
| Raw MQTT | tools/watch-mqtt.sh | Watches all escape-room MQTT traffic |
| Button-order log | tools/capture-fire-panel-buttons.sh | Saves the observed physical fire-panel button order |
| Send log | tools/setup-drive-upload.sh then tools/send_to_john.sh | Configures Drive once, then uploads the newest button-order log |

Manual fire commands:
| Command | What it does | Physical panel |
| --- | --- | --- |
| fire/status | Ask all Picos for status and wiring | STATUS |
| fire/film-on | Reveal smart film | FILM-ON |
| fire/film-off | Hide/reset smart film | FILM-OFF |
| fire/sound-look | Play check-the-oven.wav | SOUND-LOOK |
| fire/sound-crash | Play crashing_plates.m4a | SOUND-CRASH |
| fire/sound-fail | Play buzzer.mp3 | SOUND-FAIL |
| fire/sound-pass | Play yeah-you-did-it.mp3 | SOUND-PASS |
| fire/sound-bake | Play bake_at_350.wav | SOUND-BAKE |
| fire/sound-play-all | Terminal-only audio diagnostic: plays every supported file in assets/audio, sorted by filename | None |
| fire/unlock | Release final lock | UNLOCK |
| fire/reset-all | Reset all puzzles | RESET-ALL, hold 5 seconds |

Fire panel button map:
| Button | Pico GPIO | MQTT topic | Pi response | Main light feedback |
| --- | --- | --- | --- | --- |
| STATUS | GP2 | escape/fire/status | Request Pico status/wiring reports | All zones flashing red while checking, then ready states |
| FILM-ON | GP3 | escape/fire/film-on | Reveal smart film | Film flashing green |
| FILM-OFF | GP4 | escape/fire/film-off | Hide/reset smart film | Film solid green |
| SOUND-LOOK | GP5 | escape/fire/sound-look | Play check-the-oven.wav | Sound flashing green |
| SOUND-CRASH | GP6 | escape/fire/sound-crash | Play crashing_plates.m4a | Sound flashing green |
| SOUND-FAIL | GP7 | escape/fire/sound-fail | Play buzzer.mp3 | Sound flashing red |
| SOUND-PASS | GP8 | escape/fire/sound-pass | Play yeah-you-did-it.mp3 | Sound flashing green |
| SOUND-BAKE | GP9 | escape/fire/sound-bake | Play bake_at_350.wav | Sound flashing green |
| UNLOCK | GP10 | escape/fire/unlock | Release final lock | Pot/lock flashing green |
| RESET-ALL | GP11 | escape/fire/reset-all | Hold 5 seconds to reset all puzzles | Countdown flashes red, then all zones check status |

Fire panel status light map:
Pico 7 has five status-light zones, not one light per button.

| Light | Zone | Green GPIO | Red GPIO | Green Solid | Green Flashing | Red Flashing | Red/Green | Red Solid |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Film / smart film | GP12 | GP13 | Ready/hidden | Reveal active | Checking/reset | Manual diagnostic | Offline/fault |
| 2 | Sound | GP14 | GP15 | Ready | Audio playing | Wrong/checking | Manual diagnostic | Offline/fault |
| 3 | Painting | GP16 | GP17 | Ready | Painting triggered | Checking/reset | Manual diagnostic | Offline/fault |
| 4 | Color buttons | GP18 | GP19 | Ready/idle | Listening/active | Wrong/checking | Manual diagnostic | Offline/fault |
| 5 | Oven / lock | GP20 | GP21 | Ready | Oven active/unlocked/reset needed | Checking/reset countdown | Manual diagnostic | Offline/fault |
EOF
