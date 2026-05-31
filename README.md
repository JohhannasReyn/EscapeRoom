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

## Current Structure

```text
EscapeRoom/
├── assets/
│   └── crashing_plates.m4a
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

---

## Event Chain

1. Players approach the cubbies at the bottom of the stairs.
2. Pico 1 detects approach with the PIR motion detector.
3. Pico 1 publishes `escape/pico1/cubby_approach_detected`.
4. Raspberry Pi tells Pico 1 to illuminate the first cubby.
5. Players retrieve puzzle pieces.
6. Players complete the copper puzzle.
7. Pico 2 publishes `escape/pico2/copper_puzzle_complete`.
8. The copper puzzle clue points to bread.
9. The bread code opens a bottle lock.
10. The bottle message opens another padlocked box.
11. The box contains RFID cards.
12. The RFID cards unlock a cubby with the painting clue.
13. Players rotate the painting.
14. Pico 3 publishes `escape/pico3/painting_rotation_complete`.
15. Raspberry Pi plays `./assets/crashing_plates.m4a`.
16. The painting clue points players toward the final puzzle piece.
17. Players place the final puzzle piece.
18. Pico 2 publishes `escape/pico2/final_piece_placed`.
19. Raspberry Pi tells Pico 4 to reveal the smart film.
20. Transparent smart film reveals a code.
21. Players enter the code using color-coded buttons.
22. Pico 5 publishes `escape/pico5/color_sequence_complete`.
23. Raspberry Pi displays/flashes `Bake at 350 Degrees`.
24. Raspberry Pi enables Pico 4's oven knob puzzle.
25. Player turns the oven knob to 350 degrees.
26. Pico 4 reads the oven setting from the potentiometer.
27. Pico 4 updates its small LED strip as a thermometer.
28. Pico 4 unlocks the electromagnetic lock at 350 degrees within tolerance.
29. The unlocked compartment contains the key to the locked room.

---

## Raspberry Pi Controller

Project: `raspberry-pi-controller`

The controller:

- Subscribes to active Pico event topics.
- Owns the explicit room state machine.
- Logs every major state transition.
- Sends commands to Pico boards.
- Plays `./assets/crashing_plates.m4a` when the painting puzzle completes.
- Displays or flashes `Bake at 350 Degrees` when the color-button sequence completes.
- Handles whole-room reset through the Raspberry Pi reset button.

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

Display support is intentionally abstracted in `raspberry-pi-controller/src/effects/DisplayOutput.*`. The current implementation logs messages to stdout. Replace that implementation with HDMI/browser/pygame/Tkinter display code when the actual display path is chosen.

Audio playback uses `AudioEffect`. `.m4a` files are played with `ffplay` when available; other files fall back to `aplay`. If the file is missing, the controller logs the issue and continues.

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

Sound effects and TV/display messages are Raspberry Pi controller outputs, not Pico 0 pin tests. The diagnostic README includes direct Raspberry Pi checks for `crashing_plates.m4a` and the `Bake at 350 Degrees` display message path.

### Pico 1: Cubby Approach and LEDs

```text
GPIO 14 -> local reset button -> GND
GPIO 17 -> cubby addressable LED strip DIN
GPIO 6  -> PIR motion detector OUT
Pico 3V3 OUT or VBUS/5V -> PIR motion detector VCC
Pico GND -> PIR motion detector GND
```

Most HC-SR501-style PIR modules work best from 5V on VCC and output about 3.3V on OUT, which is safe for the Pico. If your specific motion detector outputs 5V on OUT, add a logic level shifter or resistor divider before connecting it to GPIO 6.

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

### Pico 3: Painting Rotation

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> RC35/reed/hall sensor output
```

The painting sensor is disabled until Raspberry Pi sends `escape/cmd/pico3/enable_painting_rotation`.

### Pico 4: Smart Film and Oven Knob

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> smart film relay/driver IN
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

Thermometer behavior is implemented through `shared/OvenThermometer.h` so the visual pattern can be tuned without repeated hardcoded LED blocks.

### Pico 5: Color Buttons

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> first configured color button
GPIO 16 -> second configured color button
```

TODO: set the real color-button sequence in `pico5-color-buttons/src/main.cpp` once the physical code is finalized:

```cpp
constexpr const char* CORRECT_SEQUENCE = "";
```

Additional color buttons can be added by extending the `buttons[]` array and assigning real GPIO pins.

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

Each Pico folder is an independent PlatformIO project. Update WiFi and broker build flags in each active `platformio.ini`:

```ini
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipicow
framework = arduino
board_build.core = earlephilhower

-D WIFI_SSID=\"YOUR_WIFI_NAME\"
-D WIFI_PASS=\"YOUR_WIFI_PASSWORD\"
-D MQTT_BROKER=\"192.168.1.172\"
```

The Pico W projects use the Earle Philhower Raspberry Pi Arduino core through the Max Gerhardt PlatformIO package. This is intentional because the standard `platformio/raspberrypi` install may not expose the `rpipicow` board on every student machine.

Build the component diagnostic project from `pico-0-component-tests/`:

```bash
pio run
```

The Raspberry Pi controller is a native Linux PlatformIO project. Install its system dependencies on the Pi:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev libgpiod-dev ffmpeg
```

Build/run the controller from `raspberry-pi-controller/`:

```bash
pio run
pio run -t exec
```

Run host-side tests from the repository root on Windows:

```powershell
.\tools\run-host-tests.ps1
```

---

## Hardware TODOs

- Confirm the PIR motion detector output voltage before wiring its OUT pin to Pico GPIO 6.
- Set the real `LEDS_PER_CUBBY` and `LEDS_BETWEEN_CUBBIES` after installing the 15m strip.
- Confirm the RC35 sensor output behavior for Pico 3 painting detection.
- Set the actual Pico 5 color-button sequence and add GPIO pins for any additional buttons.
- Calibrate `OVEN_POT_MIN_READING`, `OVEN_POT_MAX_READING`, `OVEN_TARGET_TOLERANCE`, and thermometer LED count after the oven knob is mounted.
- Replace the placeholder display implementation with the actual TV/HDMI/browser/pygame/Tkinter path.
