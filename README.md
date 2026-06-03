# Escape Room Electronics Project

This repository contains the electronics and controller code for a modular escape room project. The active room layout now uses one Raspberry Pi controller and five active Raspberry Pi Pico WH boards. A sixth Pico project folder is kept as an unused / future puzzle archive.

Core runtime pattern:

```text
Physical puzzle or sensor
-> local Pico WH detects a state change
-> Pico publishes an MQTT event over WiFi
-> Raspberry Pi controller advances the room state
-> Raspberry Pi sends commands to Picos, plays audio, or updates a display
```

The Raspberry Pi remains the central brain. Picos report state changes; the Pi coordinates the sequence.

---

## Escape Room In A Box

After the Raspberry Pi has been installed at the student site, the intended party-day behavior is:

1. Plug the Raspberry Pi into power.
2. The Pi starts the `EscapeRoom` WiFi hotspot on `wlan0`.
3. Pico boards join `EscapeRoom` and connect to MQTT at `10.42.0.1`.
4. The Pi reconnects the paired Bluetooth speaker if it is available.
5. The room controller starts automatically.
6. The HDMI TV shows the escape room dashboard on the Pi console.
7. The room waits for player/puzzle events.

The one-time install command on the Pi is:

```bash
cd /home/admin/escape-room
git pull
chmod +x tools/*.sh
tools/setup-room.sh
```

After that, normal power cycling should be plug-and-play. If a laptop is available for troubleshooting, use:

```bash
cd /home/admin/escape-room
tools/room-status.sh
tools/room-logs.sh
tools/verify-wifi.sh
```

Important script meanings:

```text
tools/monitor-puzzles.sh  Starts the actual game controller in the current terminal.
tools/watch-mqtt.sh       Watches raw MQTT messages only; it does not run the game.
tools/room-logs.sh        Watches logs from the autostarted services.
tools/tv-dashboard.sh     Shows progress on the HDMI TV/console; systemd starts it automatically.
```

Do not run `tools/monitor-puzzles.sh` manually while `escape-room-controller.service` is already running. That creates two controllers with the same MQTT client id, which can make them disconnect each other.

---

## Current Structure

```text
EscapeRoom/
├── assets/
│   └── audio/
│       ├── buzzer.mp3
│       ├── crashing_plates.m4a
│       ├── look-at-the-tv.mp3
│       ├── look-behind-you.mp3
│       └── yeah-you-did-it.mp3
├── pico-0-component-tests/            Standalone component diagnostic firmware
├── pico1-cubby-approach-leds/         Pico 1 active firmware
├── pico2-copper-final-piece/          Pico 2 active firmware
├── pico3-painting-rotation/           Pico 3 active firmware
├── pico4-smart-film-oven/             Pico 4 active firmware
├── pico5-color-buttons/               Pico 5 active firmware
├── pico6-unused-future-puzzles/       Pico 6 unused / future puzzle archive
│   └── unused-future-puzzles/
├── raspberry-pi-controller/
├── shared/
├── tests/
└── tools/
```

The Pico folder names now match the active puzzle structure. Pico 6 remains a holding area for archived puzzle code that may be restored or repurposed later.

`pico-0-component-tests` is not part of the active room runtime. It is a bench-test project for one component at a time before that component is added to a puzzle Pico.

---

## Active Pico Mapping

| Pico | Folder | Active Hardware | Responsibility |
| --- | --- | --- | --- |
| Pico 1 | `pico1-cubby-approach-leds` | 15m 5V addressable cubby LEDs, PIR motion detector | Detect cubby approach and illuminate cubbies when instructed |
| Pico 2 | `pico2-copper-final-piece` | Copper puzzle contacts, final puzzle piece contact | Report copper completion and final piece placement |
| Pico 3 | `pico3-painting-rotation` | RC35 or equivalent magnetic/reed/hall sensor | Report correct painting rotation |
| Pico 4 | `pico4-smart-film-oven` | Smart film, oven knob potentiometer, electromagnetic lock, 10-15 LED thermometer strip | Reveal smart film, track oven knob, drive thermometer, unlock at 350 |
| Pico 5 | `pico5-color-buttons` | Color-coded buttons | Report correct button sequence |
| Pico 6 | `pico6-unused-future-puzzles` | None in active runtime | Stores unused/future puzzle code |

Sensor and contact-style Pico events are edge-triggered for repeatable testing: the Pico publishes once when the input becomes active, then re-arms after the input returns inactive. The oven re-arms after the knob leaves the 350-degree target window, and the color-button Pico re-arms after a solved sequence once all buttons are released.

---

## Event Chain

1. Players approach the cubbies at the bottom of the stairs.
2. Pico 1 detects approach with the PIR motion detector.
3. Pico 1 publishes escape/pico1/cubby_approach_detected.
4. Raspberry Pi tells Pico 1 to illuminate the first 2 or 3 cubbies.
5. Players retrieve puzzle pieces & RFID cards.
6. Pico 3 publishes escape/pico3/painting_rotation_complete.
7. Raspberry Pi plays `./assets/audio/crashing_plates.m4a`.
8. Players complete the copper puzzle.
9. Pico 2 publishes escape/pico2/copper_puzzle_complete.
10. Raspberry Pi plays `./assets/audio/look-behind-you.mp3`.
11. The copper puzzle clue points to smart film.
12. Raspberry Pi tells Pico 4 to reveal the smart film.
13. The smart film code revealed to open bottle lock.
14. The bottle message opens another padlocked box.
15. The box contains colored keys for color coded locks covering utensils.
16. The utensils reveal directions for directional lock containing key to bread box.
17. Code is found hidden within the bread to color coded buttons.
18. Pico 5 publishes escape/pico5/color_sequence_complete.
19. Wrong code entered or a 10-second input timeout triggers `./assets/audio/buzzer.mp3`.
20. Correct color-button counts play `yeah-you-did-it.mp3`, then `look-at-the-tv.mp3`.
21. Raspberry Pi displays/flashes Bake at 350 Degrees.
22. Raspberry Pi enables Pico 4's oven knob puzzle.
23. Player turns the oven knob to 350 degrees.
24. Pico 4 reads the oven setting from the potentiometer.
25. Pico 4 unlocks the electromagnetic lock at 350 degrees within tolerance.
26. The unlocked compartment contains the key to the locked room.

---

## Raspberry Pi Controller

Project: `raspberry-pi-controller`

The controller:

- Subscribes to active Pico event topics.
- Owns the explicit room state machine.
- Logs every major state transition.
- Sends commands to Pico boards.
- Plays `./assets/audio/crashing_plates.m4a` when the painting puzzle completes.
- Plays `./assets/audio/look-behind-you.mp3` when the copper puzzle completes.
- Plays `./assets/audio/buzzer.mp3` when an incorrect color-button code is entered.
- Plays `./assets/audio/yeah-you-did-it.mp3`, then `./assets/audio/look-at-the-tv.mp3`, when the color-button counts are correct.
- Displays or flashes `Bake at 350 Degrees` when the color-button sequence completes.
- Handles whole-room reset through the Raspberry Pi reset button.
- Pulses a Raspberry Pi GPIO buzzer when `Bake at 350 Degrees` is displayed.
- Feeds events to an HDMI console dashboard through MQTT.

State names are defined in `shared/RoomState.h` and include:

```text
WAITING_FOR_CUBBY_APPROACH
CUBBY_APPROACH_DETECTED
FIRST_CUBBY_LIT
COPPER_PUZZLE_ACTIVE
COPPER_PUZZLE_COMPLETE
BOTTLE_LOCK_STAGE
PADLOCK_BOX_STAGE
RFID_STAGE
PAINTING_ROTATION_ACTIVE
PAINTING_ROTATION_COMPLETE
CRASHING_PLATES_PLAYED
FINAL_PIECE_ACTIVE
FINAL_PIECE_PLACED
SMART_FILM_REVEALED
COLOR_BUTTON_SEQUENCE_ACTIVE
COLOR_BUTTON_SEQUENCE_COMPLETE
DISPLAY_BAKE_350
OVEN_KNOB_ACTIVE
OVEN_TARGET_REACHED
ELECTROMAGNETIC_LOCK_RELEASED
ROOM_KEY_AVAILABLE
```

Display support is split into two layers:

- `raspberry-pi-controller/src/effects/DisplayOutput.*` logs the controller's direct display requests.
- `tools/tv-dashboard.sh` is the current production HDMI dashboard. It listens to MQTT, shows Pico readiness/progress, displays `Bake at 350 Degrees`, and shows `Well Done!` after the final lock opens.

Audio playback uses `AudioEffect`. `.m4a` and `.mp3` files are played with `ffplay` when available; other files fall back to `aplay`. If the file is missing, the controller logs the issue and continues.

The Bake-message attention buzzer uses Raspberry Pi GPIO 24 by default for 350ms. Change these defaults with build flags if the physical wiring needs different values:

```ini
-D PI_BAKE_BUZZER_GPIO=24
-D PI_BAKE_BUZZER_MS=350
```

Raspberry Pi Bake-message buzzer wiring:

```text
Raspberry Pi GPIO 24 -> active buzzer module signal/IN
Raspberry Pi GND     -> buzzer module GND
Buzzer VCC           -> 3.3V or 5V as required by the buzzer module
```

If the buzzer is a bare two-wire buzzer or draws more current than a GPIO-safe module, drive it through a transistor/MOSFET module instead of directly from GPIO.

---

## MQTT Protocol

Shared topic constants live in `shared/EscapeRoomProtocol.h`.

Events from Picos:

```text
escape/pico1/cubby_approach_detected
escape/pico1/cubby_led_ready
escape/pico1/cubby_led_error

escape/pico2/copper_puzzle_complete
escape/pico2/final_piece_placed

escape/pico3/painting_rotation_complete

escape/pico4/smart_film_ready
escape/pico4/oven_position_update
escape/pico4/oven_target_reached
escape/pico4/electromag_lock_unlocked
escape/pico4/oven_error

escape/pico5/color_sequence_complete
escape/pico5/color_sequence_error
```

Sensor telemetry from Picos:

```text
escape/telemetry/pico1/motion
escape/telemetry/pico2/contacts
escape/telemetry/pico3/painting_sensor
escape/telemetry/pico4/oven
escape/telemetry/pico5/buttons
```

The Raspberry Pi controller subscribes to `escape/telemetry/#` and prints these payloads in the terminal for live hardware testing. Telemetry is separate from game event topics, so it should not advance the room state.

Commands from Raspberry Pi:

```text
escape/cmd/pico1/enable_cubby_light
escape/cmd/pico2/enable_copper_puzzle
escape/cmd/pico3/enable_painting_rotation
escape/cmd/pico4/reveal_smart_film
escape/cmd/pico5/enable_color_button_sequence
escape/cmd/pico4/enable_oven_knob
escape/cmd/pico4/unlock_electromag_lock
escape/cmd/all/reset_puzzle
escape/cmd/all/status_request
```

Legacy compatibility topics are still recognized where practical:

```text
escape/game/reset
escape/post/query
escape/cubby/1/light_on
escape/pdlc/on
escape/oven/enable
escape/oven/degrees
escape/lock/trigger
```

---

## Pico Wiring Notes

Pin numbers are GPIO numbers, not physical header pin numbers.

### Pico 0: Component Diagnostics

Project: `pico-0-component-tests`

Use this project when a component needs to be tested by itself. Upload it to a spare Pico W, wire one component, subscribe to the debug topics from the Raspberry Pi, then select the test over MQTT.

Debug monitor:

```bash
mosquitto_sub -h localhost -v -t 'escape/debug/pico0/#'
```

Select a test:

```bash
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m ws2812
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m pir
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m magnetic_switch
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m copper_contact
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m button
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m potentiometer
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m relay_lock
mosquitto_pub -h localhost -t escape/debug/pico0/set_test -m smart_film_relay
mosquitto_pub -h localhost -t escape/debug/pico0/stop -m stop
```

When a test starts, Pico 0 publishes the expected wiring checklist to:

```text
escape/debug/pico0/wiring
```

WS2812B LED strip diagnostic wiring:

```text
5V USB pigtail + -> LED strip 5V+
5V USB pigtail - -> LED strip GND
Pico GND         -> LED strip GND
Pico GPIO 17     -> 330-470 ohm resistor -> LED strip DIN
```

The NeoPixel library is installed by PlatformIO through `lib_deps` in `pico-0-component-tests/platformio.ini`; it does not belong in `build_flags`.

The LED test lights pixels 0 through 20 one at a time every 200ms, cycles through a rainbow gradient, waits 1500ms, clears the strip, and repeats. If MQTT telemetry says the test is running but the strip stays dark, check shared ground, 5V power, DIN vs DOUT direction, GPIO 17 placement, the first LED, and whether the strip needs a 3.3V-to-5V data level shifter.

Detailed one-component-at-a-time wiring notes are in `pico-0-component-tests/include/README`.

Sound effects and TV/display messages are Raspberry Pi controller outputs, not Pico 0 pin tests. The diagnostic README includes direct Raspberry Pi checks for `crashing_plates.m4a`, `buzzer.mp3`, and the `Bake at 350 Degrees` display message path.

### Pico 1: Cubby Approach and LEDs

```text
GPIO 14 -> local reset button -> GND
GPIO 17 -> cubby addressable LED strip DIN
GPIO 6  -> PIR motion detector OUT
Pico 3V3 OUT or VBUS/5V -> PIR motion detector VCC
Pico GND -> PIR motion detector GND
```

Most HC-SR501-style PIR modules work best from 5V on VCC and output about 3.3V on OUT, which is safe for the Pico. If your specific motion detector outputs 5V on OUT, add a logic level shifter or resistor divider before connecting it to GPIO 6.

The approach event publishes once while the PIR output is HIGH, then re-arms after the PIR output returns LOW.

LED power:

```text
External 5V supply + -> LED strip 5V
External 5V supply - -> LED strip GND
External 5V supply - -> Pico GND
Pico GPIO 17        -> LED strip DIN through a 330-470 ohm resistor
```

Pico 1 power-caps the LED strip using constants near the top of `pico1-cubby-approach-leds/src/main.cpp`:

```cpp
constexpr int LED_SUPPLY_MA = 3000;
constexpr int LED_POWER_HEADROOM_MA = 500;
constexpr int LED_FULL_WHITE_CURRENT_MA = 60;
```

### Pico 2: Copper and Final Piece

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> copper puzzle complete input
GPIO 16 -> final puzzle piece placed input
```

Use a contact/switch input pattern:

```text
Pico 3.3V -> contact/switch -> Pico GPIO input
Pico GPIO input -> 10k ohm resistor -> Pico GND
```

Each contact publishes once when the input goes HIGH, then re-arms after the input returns LOW.

### Pico 3: Painting Rotation

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> RC35/reed/hall sensor output
```

The painting sensor is disabled until Raspberry Pi sends `escape/cmd/pico3/enable_painting_rotation`.

The painting event publishes once when the sensor input goes HIGH, then re-arms after the input returns LOW.

### Pico 4: Smart Film and Oven Knob

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> smart film relay/driver IN
GPIO 16 -> smart film attention buzzer driver IN
GPIO 17 -> small addressable LED thermometer strip DIN
GPIO 18 -> electromagnetic lock relay/driver IN
3.3V -> oven potentiometer outer leg
GND -> oven potentiometer outer leg
GPIO 26 / ADC0 -> oven potentiometer wiper
```

Oven constants are centralized near the top of `pico4-smart-film-oven/src/main.cpp`:

```cpp
constexpr int OVEN_MIN_VALUE = 0;
constexpr int OVEN_TARGET_VALUE = 350;
constexpr int OVEN_MAX_VALUE = 500;
constexpr int OVEN_TARGET_TOLERANCE = 10;
constexpr int OVEN_POT_MIN_READING = 0;
constexpr int OVEN_POT_MAX_READING = 4095;
constexpr int OVEN_POSITION_PUBLISH_DELTA = 2;
constexpr int THERMOMETER_LED_COUNT = 12;
```

The lock only releases when:

- Raspberry Pi has enabled the oven puzzle.
- The potentiometer-derived oven value is within tolerance of 350.

After the oven reaches 350, the Pico re-arms when the potentiometer leaves the target tolerance window.

Thermometer behavior is implemented through `shared/OvenThermometer.h` so the visual pattern can be tuned without repeated hardcoded LED blocks.

When the Raspberry Pi tells Pico 4 to reveal the smart film, Pico 4 pulses GPIO 16 for `SMART_FILM_BUZZER_MS` to alert players that the clue is visible. Use a small active buzzer module or a transistor/driver circuit. Do not connect a high-current buzzer directly to a Pico GPIO pin.

Pico 4 smart-film buzzer wiring:

```text
Pico GPIO 16 -> active buzzer module signal/IN
Pico GND     -> buzzer module GND
Buzzer VCC   -> 3.3V or 5V as required by the buzzer module
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

Pressed reads LOW; released reads HIGH.

The order of button presses does not matter. One attempt is complete after 12 total presses. The required counts are:

```text
Red    = 3 presses
Green  = 4 presses
Yellow = 2 presses
Blue   = 3 presses
```

If the 12 presses do not match those counts, Pico 5 publishes `escape/pico5/color_sequence_error`, the Raspberry Pi plays `buzzer.mp3`, and Pico 5 resets the attempt for another try. If no button is pressed for 10 seconds during an enabled attempt, Pico 5 also publishes `escape/pico5/color_sequence_error` and resets the attempt.

After a correct count is reported, Pico 5 keeps the puzzle completed until the room reset command is sent.

### Pico 6: Unused / Future Puzzle Archive

Pico 6 is not wired into the active runtime flow.

Archived source/config snapshots live here:

```text
pico6-unused-future-puzzles/unused-future-puzzles/
```

Current archive contents include the old dowels/wine, TV wall, phone/window, and blender/final firmware/config snapshots.

---

## Common Safety Notes

- Do not power locks, smart film, smoke effects, motors, or LED strips from Pico GPIO.
- Use relay modules, MOSFET modules, or isolated driver boards for high-current devices.
- Share ground between Pico and driver input side unless the driver is truly optically isolated.
- Keep player-facing electronics low voltage where possible.
- Every electronic lock should have a manual override or backup key.
- Do not switch mains voltage unless handled by someone with appropriate electrical experience.

---

## Development Setup

Open the root workspace:

```text
escape-room.code-workspace
```

Each Pico folder is an independent PlatformIO project. The active Pico projects default to the Raspberry Pi hotspot network:

```ini
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipicow
framework = arduino
board_build.core = earlephilhower

-D WIFI_SSID=\"EscapeRoom\"
-D WIFI_PASS=\"BakeAt350\"
-D MQTT_BROKER=\"10.42.0.1\"
```

The Pico W projects use the Earle Philhower Raspberry Pi Arduino core through the Max Gerhardt PlatformIO package. This is intentional because the standard `platformio/raspberrypi` install may not expose the `rpipicow` board on every student machine.

Build the component diagnostic project from `pico-0-component-tests/`:

```bash
pio run
```

---

## Raspberry Pi Operator Scripts

The `tools/` folder contains small Raspberry Pi helper scripts for setup day and party day. Run them from the Raspberry Pi unless noted otherwise.

```bash
cd /home/admin/escape-room
```

Make scripts executable after a fresh copy:

```bash
chmod +x tools/*.sh
```

Common scripts:

```text
tools/setup-room.sh           One-time install/start for travel-ready autostart.
tools/start-room.sh           Starts controller and TV dashboard services.
tools/stop-room.sh            Stops controller and TV dashboard services.
tools/room-status.sh          Shows controller and TV dashboard service status.
tools/room-logs.sh            Follows controller and TV dashboard service logs.
tools/start-wifi.sh           Creates/starts/verifies the EscapeRoom hotspot.
tools/verify-wifi.sh          Shows Ethernet, hotspot, and connected Pico clients.
tools/connect-bluetooth.sh    Reconnects the stored Bluetooth speaker and sets volume.
tools/tv-dashboard.sh         MQTT-driven HDMI TV/console dashboard.
tools/monitor-puzzles.sh      Runs the controller in the current terminal for development only.
tools/watch-mqtt.sh           Watches raw MQTT traffic for debugging only.
tools/start-venv.sh           Activates ~/.platformio-venv or opens a shell with it active.
tools/activate-pico1.sh       Test-enables Pico 1 cubby light command.
tools/activate-pico2.sh       Test-enables Pico 2 copper puzzle.
tools/activate-pico3.sh       Test-enables Pico 3 painting rotation.
tools/activate-pico4.sh       Test-enables Pico 4 smart film and oven knob.
tools/activate-pico5.sh       Test-enables Pico 5 color-button sequence.
tools/install-pi-autostart.sh Lower-level systemd installer used by setup-room.sh.
```

`tools/start-venv.sh` can be sourced:

```bash
source tools/start-venv.sh
```

If it is run normally, it opens a new shell with the PlatformIO venv active:

```bash
tools/start-venv.sh
```

Install production autostart after the Pi is configured and the controller builds:

```bash
cd /home/admin/escape-room
tools/setup-room.sh
```

After this:

- `escape-room-hotspot` is configured to autoconnect through NetworkManager.
- `escape-room-controller.service` starts the room controller at boot.
- `escape-room-controller.service` reconnects the paired Bluetooth speaker before starting the controller.
- `escape-room-tv-dashboard.service` starts the HDMI dashboard at boot.

Useful service helpers:

```bash
tools/start-room.sh
tools/stop-room.sh
tools/room-status.sh
tools/room-logs.sh
```

Do not run `pio run -e raspberry_pi_controller -t exec` in a second terminal while the service is running. Two controller instances use the same MQTT client id and can disconnect each other repeatedly.

### Raspberry Pi Hotspot Network

Use this when the escape room needs to move between houses, schools, or venues without rewriting every Pico's WiFi settings. The Raspberry Pi can host its own WiFi network, and every Pico connects to that network instead of the venue WiFi.

The recommended network values are:

```text
SSID: EscapeRoom
Password: BakeAt350
Raspberry Pi / MQTT broker IP: 10.42.0.1
```

Set up the hotspot on the Raspberry Pi:

```bash
cd /home/admin/escape-room
tools/start-wifi.sh
```

Customize the hotspot before running the script:

```bash
HOTSPOT_SSID='EscapeRoom' HOTSPOT_PASSWORD='BakeAt350' bash tools/setup-pi-hotspot.sh
```

After the hotspot is active, update each Pico `platformio.ini` to use:

```ini
-D WIFI_SSID=\"EscapeRoom\"
-D WIFI_PASS=\"BakeAt350\"
-D MQTT_BROKER=\"10.42.0.1\"
```

The Raspberry Pi controller can still use `localhost` for MQTT because Mosquitto is running on the Pi itself.

Useful hotspot commands:

```bash
nmcli connection show
nmcli connection up escape-room-hotspot
nmcli connection down escape-room-hotspot
nmcli device wifi list
tools/verify-wifi.sh
```

Recommended network layout for setup day and party day:

```text
Pi eth0  -> venue router / Mac SSH / internet
Pi wlan0 -> EscapeRoom hotspot at 10.42.0.1
Picos    -> EscapeRoom hotspot, MQTT broker 10.42.0.1
```

Healthy Pi network check:

```bash
ip -4 addr show eth0
ip -4 addr show wlan0
nmcli connection show --active
sudo iw dev wlan0 station dump
```

Expected:

```text
eth0 has a venue/router address such as 192.168.1.x
wlan0 has 10.42.0.1/24
sudo iw dev wlan0 station dump shows one station per connected Pico
```

The Raspberry Pi controller is a native Linux PlatformIO project. Install its system dependencies on the Pi:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev libgpiod-dev ffmpeg network-manager
```

### Raspberry Pi Bluetooth Audio

Use this when the Raspberry Pi should play escape room sound effects through a Bluetooth speaker.

Install and enable Bluetooth support:

```bash
sudo apt update
sudo apt install -y bluetooth bluez
sudo systemctl enable --now bluetooth
```

Pair a Bluetooth speaker:

```bash
bluetoothctl
power on
agent on
default-agent
scan on
```

Wait until the speaker appears, then copy its MAC address. It will look similar to:

```text
AA:BB:CC:DD:EE:FF Speaker Name
```

Then run these inside `bluetoothctl`, replacing the example MAC address:

```text
pair AA:BB:CC:DD:EE:FF
trust AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF
scan off
exit
```

Reconnect a previously paired speaker:

```bash
tools/connect-bluetooth.sh
```

The script defaults to the stored Yamaha ATS-1090 used during testing:

```text
BT_DEVICE=00:22:6C:12:1D:C3
BT_NAME=Yamaha ATS-1090
VOLUME=80%
```

Override those values if the speaker changes:

```bash
BT_DEVICE='AA:BB:CC:DD:EE:FF' BT_NAME='Speaker Name' VOLUME='90%' tools/connect-bluetooth.sh
```

Test project audio:

```bash
ffplay -nodisp -autoexit /home/admin/escape-room/assets/audio/buzzer.mp3
ffplay -nodisp -autoexit /home/admin/escape-room/assets/audio/crashing_plates.m4a
ffplay -nodisp -autoexit /home/admin/escape-room/assets/audio/look-behind-you.mp3
ffplay -nodisp -autoexit /home/admin/escape-room/assets/audio/yeah-you-did-it.mp3
ffplay -nodisp -autoexit /home/admin/escape-room/assets/audio/look-at-the-tv.mp3
```

If the speaker connects but no sound plays, check the active audio output:

```bash
wpctl status
```

If the Bluetooth speaker appears in the output list, set it as the default sink:

```bash
wpctl set-default SINK_ID
```

Replace `SINK_ID` with the numeric ID shown by `wpctl status`.

Set volume and unmute:

```bash
wpctl set-volume @DEFAULT_AUDIO_SINK@ 80%
wpctl set-mute @DEFAULT_AUDIO_SINK@ 0
```

If `wpctl` is missing on a different Pi image, check fallback tools:

```bash
command -v wpctl
command -v pactl
command -v amixer
```

Build/run the controller from `raspberry-pi-controller/`:

```bash
cd /home/admin/escape-room/raspberry-pi-controller
source ~/.platformio-venv/bin/activate
pio run -e raspberry_pi_controller
pio run -e raspberry_pi_controller -t exec
```

Or use the production helper:

```bash
cd /home/admin/escape-room
tools/monitor-puzzles.sh
```

### Flash and Test Each Active Pico

Use this order every time a Pico is flashed. It prevents chasing a magnet, button, or LED wiring problem before proving WiFi and MQTT are working.

1. Start the Pi hotspot and verify Ethernet is still available:

```bash
cd /home/admin/escape-room
tools/start-wifi.sh
tools/verify-wifi.sh
```

2. Start the Raspberry Pi controller in one terminal:

```bash
cd /home/admin/escape-room
tools/monitor-puzzles.sh
```

3. Start an MQTT monitor in a second terminal:

```bash
cd /home/admin/escape-room
tools/watch-mqtt.sh
```

4. On the student laptop, open the Pico project folder that matches the board being flashed. Confirm that `platformio.ini` has:

```ini
[env:pico]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipicow
framework = arduino
board_build.core = earlephilhower

build_flags =
    -D WIFI_SSID=\"EscapeRoom\"
    -D WIFI_PASS=\"BakeAt350\"
    -D MQTT_BROKER=\"10.42.0.1\"
```

5. Build, upload, and reboot the Pico:

```bash
python3 -m platformio run -t clean
python3 -m platformio run
python3 -m platformio run -t upload
```

After upload, unplug/replug the Pico.

6. On the Pi, verify the Pico joined the hotspot:

```bash
sudo iw dev wlan0 station dump
```

If this prints no station/MAC address, the Pico is not connected to WiFi yet. Recheck that the board is a Pico W/Pico WH, the correct project was uploaded, and the `EscapeRoom` credentials were compiled.

7. Verify MQTT telemetry for that Pico before testing the physical sensor:

```bash
mosquitto_sub -h localhost -v -t 'escape/telemetry/#' -t 'escape/post/cubby/+/state'
```

8. Activate the Pico under test if it normally waits for an earlier puzzle:

```bash
tools/activate-pico1.sh
tools/activate-pico2.sh
tools/activate-pico3.sh
tools/activate-pico4.sh
tools/activate-pico5.sh
```

9. Trigger the physical sensor/component and confirm both telemetry and the event topic.

Pico-specific checks:

```text
Pico 1:
  Activate: tools/activate-pico1.sh
  Telemetry: escape/telemetry/pico1/motion
  Event: escape/pico1/cubby_approach_detected

Pico 2:
  Activate: tools/activate-pico2.sh
  Telemetry: escape/telemetry/pico2/contacts
  Events: escape/pico2/copper_puzzle_complete, escape/pico2/final_piece_placed

Pico 3:
  Activate: tools/activate-pico3.sh
  Telemetry: escape/telemetry/pico3/painting_sensor
  Event: escape/pico3/painting_rotation_complete
  Expected enabled telemetry after activation: enabled=1

Pico 4:
  Activate: tools/activate-pico4.sh
  Telemetry: escape/telemetry/pico4/oven
  Events: escape/pico4/oven_target_reached, escape/pico4/electromag_lock_unlocked

Pico 5:
  Activate: tools/activate-pico5.sh
  Telemetry: escape/telemetry/pico5/buttons
  Event: escape/pico5/color_sequence_complete
```

To force the Raspberry Pi controller forward without using a physical Pico sensor, publish the Pico's event topic directly. Example for the painting puzzle:

```bash
mosquitto_pub -h localhost -t 'escape/pico3/painting_rotation_complete' -m 'painting rotation complete'
```

This advances the Pi controller, but it does not change the Pico's internal telemetry counters. Pico counters only change when the Pico firmware sees its physical input.

### TV, HDMI Audio, and Operator Feedback

The current production TV output is `tools/tv-dashboard.sh`. The autostart installer runs it as `escape-room-tv-dashboard.service` and writes it to `/dev/tty1`, which is the Raspberry Pi's HDMI console. The TV should show:

```text
Setup/POST readiness for Pico 1-5
Live puzzle progress
Bake at 350 Degrees after Pico 5 completes
Well Done! after the final electromagnetic lock opens
```

It is possible for the Pi to send one sound to the Bluetooth sound bar and a different attention sound to the HDMI TV, but the current `AudioEffect` plays to the default audio sink only. To support separate outputs cleanly, add a configurable audio-device field to `AudioEffect` and launch `ffplay` or another player against a selected PipeWire/ALSA sink. Until that is implemented, the most reliable production setup is:

```text
Default audio sink -> Bluetooth sound bar for room effects
Optional buzzer    -> Pi GPIO or Pico 4 GPIO for local attention cues
```

Current buzzer behavior:

```text
Pi GPIO 24     -> pulses when "Bake at 350 Degrees" is displayed.
Pico 4 GPIO 16 -> pulses when the smart film is activated/revealed.
```

Wire both buzzers through an appropriate module, transistor, or driver if the buzzer current is not known to be safe for GPIO. Share ground between the controller board and the buzzer driver input side.

Run host-side tests from the repository root on Windows:

```powershell
.\tools\run-host-tests.ps1
```

---

## Hardware TODOs

- Confirm the PIR motion detector output voltage before wiring its OUT pin to Pico GPIO 6.
- Set the real `LEDS_PER_CUBBY` and `LEDS_BETWEEN_CUBBIES` after installing the 15m strip.
- Confirm the RC35 sensor output behavior for Pico 3 painting detection.
- Calibrate `OVEN_POT_MIN_READING`, `OVEN_POT_MAX_READING`, `OVEN_TARGET_TOLERANCE`, and thermometer LED count after the oven knob is mounted.
- Replace the placeholder display implementation with the actual TV/HDMI/browser/pygame/Tkinter path.
