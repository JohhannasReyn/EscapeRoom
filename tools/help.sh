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
Escape room tools

Start here on the Raspberry Pi:
  tools/rebase.sh
    Downloads the latest controller, active Pico firmware, tests, tools,
    README, shared code, and pico-wifi.env. It also installs/updates
    PlatformIO in ~/.venv and builds the Raspberry Pi controller. After a
    successful update, it prints which Picos need to be flashed because their
    source changed since the last rebase.

  tools/help.sh
    Shows this guide again.

  help
    After tools/rebase.sh installs it, shows this guide from any SSH directory.

  tools/start.sh
    Starts the room controller in the terminal. Leave this running during the
    escape room. Pico wiring reports print here when Picos connect.

  tools/connect.sh
    Tests the wired speaker through the Raspberry Pi 3.5mm jack.

  tools/pair.sh
    Bluetooth is retired. This prints the wired-audio guidance.

Flashing Picos from the Pi or a Mac:
  tools/flash-pico.sh pico2
  tools/flash-pico.sh pico3
  tools/flash-pico.sh pico4
  tools/flash-pico.sh pico5
  tools/flash-pico.sh pico7
  tools/flash-pico.sh all
    Builds and uploads Pico firmware using the committed pico-wifi.env:
    SSID EscapeRoom, password BakeAt350, broker ceenypie.local.
    If you are not sure which Pico changed, run tools/rebase.sh first; it will
    say which Picos need to be flashed.
    For each Pico: unplug it, hold BOOTSEL, plug it into USB, release BOOTSEL,
    then press Enter when the script asks.

Connection and walkthrough checks:
  tools/test-connection.sh
    Requests Pico status and prints connected/not-found for each Pico.

  tools/test-all.sh
    Simulates the full room sequence with MQTT messages and delays.

  tools/test-pico1.sh
    Retired Pico 1 manual legacy test.

  tools/test-pico2.sh
    Simulates the copper puzzle completion event.

  tools/test-pico3.sh
    Simulates the painting rotation completion event.

  tools/test-pico4.sh
    Reveals smart film, watches oven potentiometer telemetry while the knob is
    turned, and simulates oven completion/unlock.

  tools/test-pico5.sh
    Simulates the wrong-code buzzer event, checks Pico 5 telemetry, and
    simulates color button sequence completion.

  tools/capture-fire-panel-buttons.sh
    Guides the student through each fire-panel button and saves the observed
    MQTT order to a file they can send back for review/remapping.

  tools/setup-drive-upload.sh
    Saves the shared Google Drive folder settings to john-contact.env and shows
    the rclone setup steps for the Pi. Run this once before send_to_john if the
    Pi has not been connected to the shared folder yet.

  tools/send_to_john.sh
    Finds the newest fire-panel button-order log and uploads it to the shared
    Google Drive folder with rclone if configured. If Drive is not configured,
    it can fall back to upload URL, SCP, email, or a small -for-john.tar.gz
    bundle to send manually.

  tools/run-host-tests.sh
    Builds/runs host-side logic tests on macOS, Linux, or Raspberry Pi.

Audio:
  tools/set-volume.sh -v 50
    Sets speaker volume to 50%.

  tools/volume-up.sh
    Increases volume by 10%.

  tools/volume-down.sh
    Decreases volume by 10%.

  The room controller queues audio and plays one cue at a time so MQTT sensor
  messages and reset commands stay responsive during sound playback.

Room service helpers:
  tools/setup-room.sh
    Makes scripts executable, installs the Pi autostart service, and restarts
    the room controller. Run this as a normal command from inside escape-room;
    do not run it with source.

  tools/install-pi-autostart.sh
    Installs/updates the systemd service that starts the room controller.

  tools/start-room.sh
    Starts the systemd room service.

  tools/stop-room.sh
    Stops the systemd room service.

  tools/room-status.sh
    Shows whether the room service is running.

  tools/room-logs.sh
    Watches systemd logs for the room service.

Debug:
  tools/watch-mqtt.sh
    Watches raw MQTT messages.

  tools/monitor-puzzles.sh
    Watches puzzle-related MQTT messages.

  tools/start-venv.sh
    Opens the PlatformIO virtual environment shell.

Internal helpers:
  tools/lib-room.sh
    Shared functions used by the test scripts.

  tools/platformio_pico_wifi.py
    PlatformIO pre-build hook that compiles pico-wifi.env into Pico firmware.

  tools/install-help-command.sh
    Installs the SSH-wide help command by adding an escape-room help function
    to ~/.bashrc.

  john-contact.env.example
    Template for the shared Google Drive folder, optional upload URL, SCP, or
    email settings used by tools/send_to_john.sh. Copy it to john-contact.env
    for local settings, or run tools/setup-drive-upload.sh.

  tools/run-host-tests.ps1
    Windows PowerShell version of the host-side tests.

Fire panel fallbacks:
  fire/status
    Ask all Picos to report state and wiring.

  fire/film-on
    Reveal smart film.

  fire/film-off
    Hide/reset smart film.

  fire/sound-look
    Play check-the-oven audio.

  fire/sound-crash
    Play crashing plates audio.

  fire/sound-fail
    Play buzzer audio.

  fire/sound-pass
    Play success audio.

  fire/sound-bake
    Play bake-at-350 audio.

  fire/unlock
    Release final lock.

  fire/reset-all
    Reset all puzzles. The physical panel reset requires a 5-second hold.

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

Light state key:
| Light state | Meaning |
| --- | --- |
| Solid green | Ready, reset, or idle/OK |
| Flashing green | Active, playing, triggered, unlocked, or physical reset needed |
| Flashing red | Checking, wrong, error, or reset countdown |
| Alternating red/green | Reserved diagnostic/manual LED command: alternating-red-green |
| Solid red | Offline/fault/manual LED command: solid-red |
EOF
