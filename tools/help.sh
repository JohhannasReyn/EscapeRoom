#!/usr/bin/env bash
set -euo pipefail

cat <<'EOF'
Escape room tools

Start here on the Raspberry Pi:
  tools/rebase.sh
    Downloads the latest controller, active Pico firmware, tests, tools,
    README, shared code, and pico-wifi.env. It also installs/updates
    PlatformIO in ~/.venv and builds the Raspberry Pi controller.

  tools/help.sh
    Shows this guide again.

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
    Reveals smart film and simulates oven completion/unlock.

  tools/test-pico5.sh
    Simulates color button sequence completion.

  tools/run-host-tests.sh
    Builds/runs host-side logic tests on macOS, Linux, or Raspberry Pi.

Audio:
  tools/set-volume.sh -v 50
    Sets speaker volume to 50%.

  tools/volume-up.sh
    Increases volume by 10%.

  tools/volume-down.sh
    Decreases volume by 10%.

Room service helpers:
  tools/setup-room.sh
    Makes scripts executable and installs the Pi autostart service.

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
EOF
