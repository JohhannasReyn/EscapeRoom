# Escape Room Electronics Project

This repository contains the Raspberry Pi controller, Raspberry Pi Pico WH firmware, shared MQTT protocol, audio assets, and field tools for the escape room.

The current build uses:

- One Raspberry Pi as the central controller and MQTT broker.
- Five active Pico WH boards on the same local WiFi as the Pi.
- No TV dashboard (the controller logs display/cue messages such as "Bake at 350 Degrees" to the console only).
- A powered speaker connected to the Raspberry Pi 3.5mm audio jack for audio cues.
- A one-piece copper puzzle that advances directly to the smart-film stage.

Core runtime pattern:

```text
Physical puzzle or sensor
-> local Pico WH detects a state change
-> Pico publishes an MQTT event over local WiFi
-> Raspberry Pi controller advances room state
-> Raspberry Pi sends commands to Picos or plays audio
```

---

## Network Design

The Raspberry Pi still hosts MQTT, but it no longer creates an ad-hoc/hotspot network. The Pi and all Picos should join the venue's normal WiFi.

Keep the existing Pi SSH identity:

```text
admin@ceenypie.local
```

Picos should use:

```text
MQTT_BROKER=ceenypie.local
```

Use a router DHCP reservation for the Pi if available. The shared Pico config also supports a fallback IP if local name resolution has trouble.

Shared Pico WiFi/MQTT settings live in:

```text
pico-wifi.env
```

Example:

```bash
WIFI_SSID="VenueWifi"
WIFI_PASS="VenuePassword"
MQTT_BROKER="ceenypie.local"
MQTT_BROKER_FALLBACK="192.168.1.50"
MQTT_BROKER_PORT="1883"
```

`pico-wifi.env` is git-ignored, so it does not ship in the repo. Create it once by copying the template, then edit it:

```bash
cp pico-wifi.env.example pico-wifi.env
```

Edit `pico-wifi.env` before flashing Picos. Each active Pico project reads this file during PlatformIO builds (`tools/platformio_pico_wifi.py`), and the build fails with "Missing shared Pico WiFi config" if the file is absent.

Optional Pi-side WiFi notes can be copied from:

```text
pi-wifi.env.example
```

---

## Initial Pi Setup

On the Raspberry Pi:

```bash
cd /home/admin/escape-room
git pull
chmod +x tools/*.sh
```

Install the controller dependencies if needed:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev libgpiod-dev ffmpeg alsa-utils pipewire wireplumber
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

Set up PlatformIO on the Pi:

```bash
python3 -m venv ~/.venv
source ~/.venv/bin/activate
python -m pip install -U pip platformio
```

Start the room controller in the terminal:

```bash
cd /home/admin/escape-room
tools/start.sh
```

The controller prints puzzle events and telemetry changes only. Repeated identical telemetry is suppressed to keep the terminal readable.

---

## Flashing Picos

Before flashing, edit root `pico-wifi.env` with the venue WiFi SSID/password and the Pi broker host.

For each Pico:

```text
Open VS Code -> Select Terminal -> New Terminal -> Select the correct Pico folder to flash
```

Create and activate a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install PlatformIO:

```bash
python -m pip install -U pip platformio
```

Build and upload:

```bash
pio run
pio run -t upload
```

If upload detection fails, plug the Pico in while holding `BOOTSEL`, then run `pio run -t upload` again.

---

## Active Pico Mapping

| Pico | Folder | Active Hardware | Responsibility |
| --- | --- | --- | --- |
| Pico 1 | `pico1-cubby-approach-leds` | Retired | Previous cubby LEDs and PIR motion detector |
| Pico 2 | `pico2-copper-final-piece` | One-piece copper puzzle contact | Report copper completion and advance smart-film stage |
| Pico 3 | `pico3-painting-rotation` | Painting rotation sensor | Report picture rotation |
| Pico 4 | `pico4-smart-film-oven` | Smart film, oven knob, electromagnetic lock | Reveal film, track oven, unlock at 350 |
| Pico 5 | `pico5-color-buttons` | Color-coded buttons | Report correct button sequence |
| Pico 6 | `pico6-unused-future-puzzles` | None | Archive/future code |
| Pico 7 | `pico7-fire-panel` | 10-button fire panel with red/green status LEDs | Operator fallback controls and state display |

Pin numbers are GPIO numbers, not physical header pin numbers.

---

## Current Game Flow

When the controller enters ready state, Pico 2, Pico 3, and Pico 5 are armed:

1. Pico 2 waits for the puzzle piece placement.
2. Pico 3 actively listens for the picture rotation magnet sensor.
3. Pico 5 actively listens for the color-button combination.
4. Pico 4 reports oven telemetry, but the oven knob puzzle remains disabled.

Active room progression:

1. Pico 2 publishes `escape/pico2/copper_puzzle_complete` when the puzzle piece is placed.
2. Raspberry Pi plays `check-the-oven.wav`.
3. Raspberry Pi tells Pico 4 to reveal the smart film.
4. If Pico 3 publishes `escape/pico3/painting_rotation_complete`, Raspberry Pi plays `crashing_plates.m4a` once for that sensor event.
5. If Pico 5 publishes `escape/pico5/color_sequence_error`, Raspberry Pi plays `buzzer.mp3`.
6. Pico 5 publishes `escape/pico5/color_sequence_complete` after the correct combo.
7. Raspberry Pi plays `yeah-you-did-it.mp3`, then `bake_at_350.wav`, then enables Pico 4's oven knob.
8. If the oven is already at 350 when the room starts or when the knob is enabled, the fire panel's potentiometer status flashes green until the knob is moved away from 350.
9. Player turns the oven knob to 350 and holds it there.
10. Pico 4 publishes `escape/pico4/oven_target_reached`.
11. Raspberry Pi tells Pico 4 to unlock the electromagnetic lock.

The electromagnetic lock stays released (Pico 4 holds `GPIO 18` HIGH) until the
room is reset, so the door remains open for the group. A fire-panel `reset-all`
or the Pi reset button re-locks the door, re-arms Pico 2/3/5, disables the oven
knob, and clears the controller's one-shot cues (including the once-per-game
crashing-plates sound) so the next group gets a clean run.

The legacy final-piece topic still exists for manual testing, but it is no longer required in the active flow.

The `escape/post/cubby/+/state` handshake and `escape/cubby/N/...` status topics
are carried over from the old multi-cubby room. They are inert in the current
one-piece build (no cubby 1 reporter exists, so the POST "all green" path never
fires) and can be ignored.

---

## Wiring Notes

### Pico 1: Retired Cubby Approach and LEDs

Pico 1 is no longer part of the active room flow. The old firmware remains in
the repository for reference only.

```text
GPIO 14 -> local reset button -> GND
GPIO 17 -> cubby addressable LED strip DIN
GPIO 6  -> PIR motion detector OUT
Pico GND -> PIR motion detector GND
Pico 3V3 OUT or VBUS/5V -> PIR motion detector VCC
```

LED power:

```text
External 5V supply + -> LED strip 5V
External 5V supply - -> LED strip GND
External 5V supply - -> Pico GND
Pico GPIO 17        -> LED strip DIN through a 330-470 ohm resistor
```

### Pico 2: One-Piece Copper Puzzle

Pico 2 uses the Pico's internal pull-up resistor. The puzzle contact only needs to connect the input pin to GND when the piece is placed:

```text
GPIO 15 -> puzzle contact -> GND
```

The active controller flow only needs GPIO 15:

```text
GPIO 15 LOW -> escape/pico2/copper_puzzle_complete
```

GPIO 16 can remain wired the same way for legacy/manual final-piece behavior:

```text
GPIO 16 -> optional legacy final-piece contact -> GND
```

No external 10k resistor is needed for Pico 2 puzzle inputs.

### Pico 3: Painting Rotation

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> RC35/reed/hall sensor output (drives the pin HIGH when the painting is rotated into position)
```

This Pico is armed when the room reaches ready and actively listens for the
painting rotation magnet sensor. Rotating the painting into position publishes
`escape/pico3/painting_rotation_complete`, and the Raspberry Pi plays the
crashing-plates cue once per game.

### Pico 4: Smart Film and Oven Knob

```text
GPIO 14 -> local reset button -> GND
GPIO 15 / physical pin 20 -> smart film relay/driver IN
GPIO 16 / physical pin 21 -> smart film reveal buzzer + (buzzer - -> GND)
GPIO 18 / physical pin 24 -> electromagnetic lock relay/driver IN
3.3V -> oven potentiometer outer leg
GND -> oven potentiometer outer leg
GPIO 26 / ADC0 / physical pin 31 -> oven potentiometer wiper
```

### Pico 5: Color Buttons

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> red button
GPIO 16 -> green button
GPIO 17 -> yellow button
GPIO 18 -> blue button
```

Each button uses the Pico's internal pull-up resistor:

```text
Pico GPIO input -> button -> Pico GND
```

Required counts:

```text
Red    = 3 presses
Green  = 4 presses
Yellow = 2 presses
Blue   = 3 presses
```

---

## Tools

Make scripts executable:

```bash
chmod +x tools/*.sh fire/*
```

Show available tools:

```bash
tools/help.sh
```

Start the room:

```bash
tools/start.sh
```

Wired speaker:

```bash
tools/connect.sh
tools/set-volume.sh -v 50
tools/volume-up.sh
tools/volume-down.sh
```

Connectivity and walkthrough tests:

```bash
tools/test-connection.sh
tools/test-all.sh
tools/test-pico2.sh
tools/test-pico3.sh
tools/test-pico4.sh
tools/test-pico5.sh
```

Raw MQTT/debug:

```bash
tools/watch-mqtt.sh
tools/room-logs.sh
```

Manual fire commands:

```bash
fire/status
fire/film-on
fire/film-off
fire/sound-look
fire/sound-crash
fire/sound-fail
fire/sound-pass
fire/sound-bake
fire/unlock
fire/reset-all
```

The physical fire-panel reset button must be held for 5 seconds. During the hold,
the five red LEDs flash one at a time as a countdown; after reset fires, all five
red LEDs flash together and the controller requests status before re-arming the
room.

Update Pi-side folders from the public repo and rebuild:

```bash
tools/rebase.sh
```

---

## MQTT Topics

Important Pico events:

```text
escape/pico2/copper_puzzle_complete
escape/pico2/final_piece_placed          # legacy/manual
escape/pico3/painting_rotation_complete
escape/pico4/oven_target_reached
escape/pico4/electromag_lock_unlocked
escape/pico5/color_sequence_complete
escape/pico5/color_sequence_error
```

Commands from the Pi:

```text
escape/cmd/pico2/enable_copper_puzzle
escape/cmd/pico4/reveal_smart_film
escape/cmd/pico5/enable_color_button_sequence
escape/cmd/pico4/enable_oven_knob
escape/cmd/pico4/unlock_electromag_lock
escape/cmd/all/reset_puzzle
escape/cmd/all/status_request
escape/cmd/fire-panel/led
```

Fire panel events:

```text
escape/fire/status
escape/fire/film-on
escape/fire/film-off
escape/fire/sound-look
escape/fire/sound-crash
escape/fire/sound-fail
escape/fire/sound-pass
escape/fire/sound-bake
escape/fire/unlock
escape/fire/reset-all
```

Telemetry:

```text
escape/telemetry/pico2/contacts
escape/telemetry/pico3/painting_sensor
escape/telemetry/pico4/oven
escape/telemetry/pico5/buttons
escape/telemetry/fire-panel/status
```

The controller subscribes to telemetry but only prints it when the payload changes.
