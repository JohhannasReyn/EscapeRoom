# Escape Room Electronics Project

This repository contains the electronics and controller code for a modular escape room project built around one Raspberry Pi controller and six Raspberry Pi Pico WH puzzle controllers.

The core pattern is:

```text
Physical puzzle/sensor
-> local Pico WH detects the state
-> Pico publishes an MQTT event over WiFi
-> Raspberry Pi controller receives the event
-> Raspberry Pi publishes the next command or triggers an effect
```

The room currently includes contact puzzles, a distance-sensor stair trigger, cubby LEDs, POST/reset behavior, smart film, smoke/lock outputs, phone/window props, a blender/final output, and an oven dial final puzzle.
(**June 6th Deadline**)

---

## Current Project Structure

```text
EscapeRoom/
├── assets/
│   └── .gitkeep
├── docs/
│   └── .gitkeep
├── README.md
├── escape-room.code-workspace
├── escape-room-pico/
│   ├── platformio.ini
│   ├── include/
│   ├── lib/
│   └── src/
│       ├── CubbyLedLayout.h
│       └── main.cpp
├── pico-cabinet-dowels-wine/
│   ├── platformio.ini
│   └── src/main.cpp
├── pico-fireplace-reveal-effects/
│   ├── platformio.ini
│   └── src/main.cpp
├── pico-tv-wall/
│   ├── platformio.ini
│   └── src/main.cpp
├── pico-phone-window-props/
│   ├── platformio.ini
│   └── src/main.cpp
├── pico-back-room-blender-final/
│   ├── platformio.ini
│   └── src/main.cpp
├── shared/
│   ├── EncoderDial.h
│   ├── LedPowerBudget.h
│   ├── PostState.h
│   ├── PulseTimer.h
│   └── PuzzleDebounce.h
├── tests/
│   ├── README.md
│   ├── encoder_dial_test.cpp
│   ├── led_power_budget_test.cpp
│   ├── pico_post_mapping_test.cpp
│   ├── post_state_test.cpp
│   ├── pulse_timer_test.cpp
│   └── puzzle_debounce_test.cpp
├── tools/
│   └── run-host-tests.ps1
└── raspberry-pi-controller/
    ├── platformio.ini
    ├── test/
    └── src/
        ├── GameController.cpp
        ├── GameController.h
        ├── PuzzleModule.h
        ├── ResetControl.h
        ├── effects/
        ├── puzzles/
        └── main.cpp
```

---

## Projects

### `escape-room-pico`

This project runs on a Raspberry Pi Pico WH near the stair/cubby/entry area.

Responsibilities:

* Read puzzle sensors
* Detect completed puzzle states
* Debounce physical inputs
* Connect to WiFi
* Publish MQTT events to the Raspberry Pi

Current behavior:

```text
Copper/contact puzzle solved
→ Pico detects GPIO input
→ Pico publishes:
   escape/puzzle/copper/solved

Stairs trigger activated
→ Pico detects GPIO input
→ Pico publishes:
   escape/puzzle/stairs/triggered

Cubby commands received
→ Pico receives MQTT command
→ Pico turns individual cubby LED-strip segments or lock relay driver on/off

Startup/POST lights
→ Pico sweeps cubbies 1 through 6
→ Pico holds all cubbies white
→ Raspberry Pi asks every Pico to report whether its puzzle is ready or still completed
→ Raspberry Pi marks ready puzzle cubbies green and completed puzzle cubbies red
→ When every cubby has reported ready, Raspberry Pi sends ready/off command
→ Pico clears lights and waits for gameplay
```

---

## Current Puzzle Inventory

These are the active puzzle/effect modules represented in the current project folders.

| Zone | Project | Puzzle or Effect | Main MQTT Topics |
| --- | --- | --- | --- |
| Entry / stairs / cubbies | `escape-room-pico` | Copper begun contact | `escape/puzzle/copper/begun` |
| Entry / stairs / cubbies | `escape-room-pico` | Copper solved contact | `escape/puzzle/copper/solved` |
| Entry / stairs / cubbies | `escape-room-pico` | Stairs distance trigger | `escape/puzzle/stairs/triggered` |
| Entry / stairs / cubbies | `escape-room-pico` | Cubby addressable LEDs and cubby 2 lock | `escape/cubby/#/light_on`, `escape/cubby/#/status`, `escape/cubby/2/unlock` |
| Cabinet / shelf | `pico-cabinet-dowels-wine` | Dowels puzzle | `escape/puzzle/dowels/solved` |
| Cabinet / shelf | `pico-cabinet-dowels-wine` | Wine puzzle | `escape/puzzle/wine/solved` |
| Fireplace / reveal | `pico-fireplace-reveal-effects` | Fireplace puzzle | `escape/puzzle/fireplace/solved` |
| Fireplace / reveal | `pico-fireplace-reveal-effects` | Smart film, smoke burst, electromagnetic lock | `escape/pdlc/on`, `escape/smoke/burst`, `escape/lock/trigger` |
| Fireplace / reveal | `pico-fireplace-reveal-effects` | Oven dial final puzzle | `escape/oven/enable`, `escape/oven/degrees`, `escape/puzzle/oven/solved` |
| TV wall | `pico-tv-wall` | TV intro trigger | `escape/tv/play_intro` |
| Right wall / window | `pico-phone-window-props` | Phone puzzle | `escape/puzzle/phone/solved` |
| Right wall / window | `pico-phone-window-props` | Window/right-wall prop | `escape/puzzle/window/triggered` |
| Back room / final | `pico-back-room-blender-final` | Blender flour puzzle | `escape/puzzle/blender/solved` |
| Back room / final | `pico-back-room-blender-final` | Final win-condition output | `escape/game/win` |
| Whole room | all Picos + Raspberry Pi | POST and reset | `escape/post/query`, `escape/post/cubby/#/state`, `escape/game/reset` |

### Pico project grouping

There are six deployable Pico WH projects. Puzzles are grouped by physical proximity so the wiring stays local and each Pico still has GPIO headroom for reset buttons, status LEDs, and future inputs.

```text
escape-room-pico/
  Pico 1: stair / under-stair / entry zone
  Inputs: copper solved contact, optional copper begun contact, stairs laser distance sensor
  Outputs: addressable cubby LED strip, cubby lock relay driver
  Publishes:
    escape/puzzle/copper/begun
    escape/puzzle/copper/solved
    escape/puzzle/stairs/triggered
  Subscribes:
    escape/cubby/1/light_on
    escape/cubby/2/light_on
    escape/cubby/3/light_on
    escape/cubby/4/light_on
    escape/cubby/5/light_on
    escape/cubby/6/light_on
    escape/cubby/1/status
    escape/cubby/2/status
    escape/cubby/3/status
    escape/cubby/4/status
    escape/cubby/5/status
    escape/cubby/6/status
    escape/cubby/all/status
    escape/cubby/2/unlock
    escape/post/query
    escape/game/reset

pico-cabinet-dowels-wine/
  Pico 2: left wall shelf / cabinet area
  Inputs: dowels puzzle, wine puzzle
  Publishes:
    escape/puzzle/dowels/solved
    escape/puzzle/wine/solved
  Subscribes:
    escape/game/reset

pico-fireplace-reveal-effects/
  Pico 3: fireplace / reveal zone
  Inputs: fireplace log puzzle, oven dial rotary encoder
  Outputs: PDLC smart film, smoke burst, electric lock trigger
  Publishes:
    escape/puzzle/fireplace/solved
    escape/puzzle/oven/solved
    escape/oven/degrees
  Subscribes:
    escape/pdlc/on
    escape/smoke/burst
    escape/lock/trigger
    escape/oven/enable
    escape/game/reset

pico-tv-wall/
  Pico 4: TV wall
  Outputs: TV intro trigger
  Subscribes:
    escape/tv/play_intro
    escape/game/reset

pico-phone-window-props/
  Pico 5: right wall / window props
  Inputs: phone puzzle, right-wall/window prop
  Publishes:
    escape/puzzle/phone/solved
    escape/puzzle/window/triggered
  Subscribes:
    escape/game/reset

pico-back-room-blender-final/
  Pico 6: back room / final zone
  Inputs: blender flour puzzle
  Outputs: final win-condition relay/driver
  Publishes:
    escape/puzzle/blender/solved
  Subscribes:
    escape/game/win
    escape/game/reset
```

The output-heavy Pico projects should use relay modules, MOSFET driver modules, or isolated driver boards. If the number of cubby lights, locks, window props, or reveal effects grows, add an I2C GPIO expander such as an MCP23017 instead of trying to crowd every device onto direct Pico GPIO.

Pico 1 uses one continuous addressable LED strip for the cubbies. The firmware has setup constants near the top of `escape-room-pico/src/main.cpp`:

```cpp
constexpr int CUBBY_COUNT = 6;
constexpr int LEDS_PER_CUBBY = 30;
constexpr int LEDS_BETWEEN_CUBBIES = 3;
constexpr int CUBBY_LED_BRIGHTNESS = 80;
constexpr int CUBBY_ACTIVE_DISTANCE_MM = 650;
constexpr int STARTUP_CUBBY_STEP_MS = 300;
constexpr int STARTUP_ALL_WHITE_MS = 800;
constexpr int LED_SUPPLY_MA = 3000;
constexpr int LED_POWER_HEADROOM_MA = 500;
constexpr int LED_FULL_WHITE_CURRENT_MA = 60;
```

After the lights are physically installed, count the real LEDs in one cubby and the real LEDs in each gap, then update those values.

The LED strip is power-capped in firmware for a 5V 3A USB adapter. The code reserves `LED_POWER_HEADROOM_MA` and only lets the cubby LEDs use the remaining budget. If multiple cubbies are on and the estimated current would exceed the budget, Pico 1 automatically lowers NeoPixel brightness before calling `show()`. Typical WS2812-style LEDs are about 60mA, or 0.06A, per LED at full white. If the exact LED strip datasheet gives a different number, update `LED_FULL_WHITE_CURRENT_MA`.

On Pico startup, the cubbies sweep on in order, then all turn white. The Raspberry Pi uses the same strip as a startup POST display by asking every Pico to report its puzzle state on `escape/post/query`.

Each Pico replies with one or more cubby state reports:

```text
escape/post/cubby/1/state -> ready or completed
escape/post/cubby/2/state -> ready or completed
escape/post/cubby/3/state -> ready or completed
escape/post/cubby/4/state -> ready or completed
escape/post/cubby/5/state -> ready or completed
escape/post/cubby/6/state -> ready or completed
```

The Raspberry Pi turns a cubby green when the reported state is `ready` and red when the reported state is `completed`. Red means the operator should physically return that puzzle to its ready state, then press that Pico's local reset button. The Pico will publish its updated POST state after the local reset.

The operator can also press the Raspberry Pi whole-room reset button after returning the physical puzzle to its ready state. The Pi publishes `escape/game/reset`, waits briefly, and repeats the POST query. When all six cubbies have reported `ready`, the Pi sends `escape/cubby/all/status` with `off`; that clears the strip and puts Pico 1 into its ready state. The stairs distance sensor is ignored until this ready command arrives.

### Room Reset Button

The Raspberry Pi controller can host a physical whole-room reset button. Wire the button like this:

```text
Raspberry Pi GPIO 23 -> reset button -> GND
```

The controller requests GPIO 23 with an internal pull-up through `libgpiod`, so the button reads:

```text
not pressed = HIGH
pressed     = LOW
```

Hold the button for one second to publish:

```text
escape/game/reset
```

Every Pico subscribes to that topic. When received, each Pico clears local solved-state flags and turns off local outputs that it owns.

---

### `raspberry-pi-controller`

This project runs on the Raspberry Pi 4.

Responsibilities:

* Run the main game controller
* Listen for MQTT puzzle events
* Trigger audio, video, lights, locks, smart film, smoke effects, or other outputs
* Track game state as the room grows

Current behavior:

```text
On startup:
-> connect to local Mosquitto broker
-> subscribe to every registered puzzle topic
-> subscribe to POST reports and oven dial telemetry
-> query every Pico with escape/post/query

During POST:
-> completed puzzle reports turn matching cubbies red
-> ready puzzle reports turn matching cubbies green
-> when all six cubbies report ready, Pi sends escape/cubby/all/status -> off

During gameplay:
-> copper solved triggers the crash audio effect
-> dowels, wine, fireplace, phone, and blender events light the matching cubbies
-> after all required puzzles are solved, Pi unlocks/enables the oven dial
-> oven solved publishes escape/game/win

Whole-room reset:
-> hold Raspberry Pi GPIO 23 reset button for one second
-> Pi publishes escape/game/reset
-> Pi repeats POST query so the operator can see any still-completed puzzle in red
```

Controller code layout:

```text
src/main.cpp
  Sets up Mosquitto MQTT, subscribes to puzzle topics, watches the Pi reset button,
  and publishes queued commands from the game controller.

src/GameController.*
  Owns puzzle modules, routes MQTT topics to the matching class, tracks POST state,
  stores oven dial telemetry, gates the oven unlock, and queues outgoing commands.

src/puzzles/
  Contains the copper puzzle module plus one topic class for each planned/current puzzle.

src/effects/
  Contains reusable output actions such as audio playback.
```

---

## Hardware Used So Far

### Main controllers

* Raspberry Pi 4 Model B
* Raspberry Pi Pico WH
* Arduino Uno R3, optional for later puzzles

### Prototype parts

* Breadboard
* Jumper wires
* Copper tape or foil
* 10kΩ resistor
* Pico onboard LED
* Raspberry Pi audio output
* Optional speaker

### Larger escape-room parts available

* Pico WH boards
* Relay modules
* Arduino relay shield
* Electromagnetic locks
* LED reel
* 12V adapter
* Smart film
* Miscellaneous locks
* Breadboards
* Soldering kit
* Plastic enclosures

---

## Software Used

### Development

* VS Code
* PlatformIO

### Pico WH firmware

* C++
* Arduino framework through PlatformIO
* PubSubClient MQTT library

### Raspberry Pi controller

* C++
* PlatformIO `native` environment
* Mosquitto MQTT library

### Raspberry Pi services

* Mosquitto MQTT broker
* `aplay` for simple audio playback

---

## Dependency Setup

This project has two kinds of dependencies.

### Pico projects

The Pico WH projects use Arduino libraries that PlatformIO can download from the PlatformIO Library Registry.

Each Pico project has this in its `platformio.ini`:

```ini
lib_deps =
    knolleary/PubSubClient@^2.8
```

When the student opens a Pico project and builds/uploads it with PlatformIO, PlatformIO downloads `PubSubClient` automatically if it is not already installed.

Pico 1 also uses addressable LEDs and a VL53L0X-style laser distance sensor:

```ini
lib_deps =
    knolleary/PubSubClient@^2.8
    adafruit/Adafruit NeoPixel@^1.12.5
    pololu/VL53L0X@^1.3.1
```

PlatformIO will download these from the PlatformIO Library Registry during the first build.

### Raspberry Pi controller

The Raspberry Pi controller is different. It is a native Linux C++ program, so its MQTT and GPIO dependencies are Raspberry Pi OS packages, not Arduino libraries.

These packages cannot be replaced by PlatformIO `lib_deps`:

```text
mosquitto
mosquitto-clients
libmosquitto-dev
libgpiod-dev
```

Install them on the Raspberry Pi before building the controller:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev libgpiod-dev
```

The controller `platformio.ini` should only link the installed system libraries:

```ini
build_flags =
    -std=c++17
    -lmosquitto
    -lgpiod
```

Do not put Debian package names such as `libmosquitto-dev` or `mosquitto-clients` in `build_flags`; PlatformIO will treat them like linker flags, not install commands.

---

## MQTT Event Flow

The Pico and Raspberry Pi communicate using MQTT.

The Pico publishes an event when a puzzle is solved.

Example topic:

```text
escape/puzzle/copper/solved
```

The Raspberry Pi subscribes to that topic and triggers the next effect.

Basic flow:

```text
Pico WH
  publishes MQTT message
        ↓
Raspberry Pi Mosquitto broker
        ↓
Raspberry Pi game controller
        ↓
Audio/light/effect trigger
```

---

## First Prototype Wiring

Copper puzzle input:

```text
Pico 3.3V -> copper solved contact side A

copper solved contact side B -> GPIO 15

GPIO 15 -> 10k ohm resistor -> GND
```

When the copper puzzle piece bridges the solved contact, GPIO 15 reads HIGH and publishes `escape/puzzle/copper/solved`.

Optional copper begun input:

```text
Pico 3.3V -> copper begun contact side A
copper begun contact side B -> GPIO 19
GPIO 19 -> 10k ohm resistor -> GND
```

If the begun contact is not installed, set this near the top of `escape-room-pico/src/main.cpp`:

```cpp
constexpr bool COPPER_STARTED_INPUT_ENABLED = false;
```

Optional reset button:

```text
GPIO 14 → Button → GND
```

GPIO 14 uses the Pico internal pull-up resistor.

---

## Room Wiring Guide

These wiring notes match the current project code. Pin numbers below are **GPIO numbers**, not physical header pin numbers, unless a physical pin is explicitly stated.

### Common Pico Input Pattern

Use this pattern for contact sensors, reed switches, pressure mats, simple prop switches, and solved-state outputs from another low-voltage circuit:

```text
Pico 3.3V -> switch/contact/sensor -> Pico GPIO input
Pico GPIO input -> 10k ohm resistor -> Pico GND
```

When the switch closes, the GPIO reads `HIGH`. When open, the 10k resistor pulls the GPIO to `LOW`.

Use this pattern for:

```text
GPIO 15: copper, dowels, blender, phone, or TV trigger depending on project
GPIO 16: stairs, wine, final output, or window prop depending on project
GPIO 17: fireplace puzzle input on the fireplace Pico
```

### Common Pico Local Reset Button

Most Pico puzzle projects also support a local reset button:

```text
Pico GPIO 14 -> reset button -> Pico GND
```

GPIO 14 uses the Pico internal pull-up resistor:

```text
not pressed = HIGH
pressed     = LOW
```

The whole-room reset button is different. It is wired to the Raspberry Pi and sends `escape/game/reset` over MQTT to every Pico.

### Common Pico Output Pattern

Do not power locks, LED strips, smart film, smoke effects, or other high-current devices directly from Pico GPIO.

Use a relay module, MOSFET module, or isolated driver board:

```text
Pico GPIO output -> driver module IN
Pico GND         -> driver module GND
Driver VCC       -> driver logic voltage, usually 3.3V or 5V depending on module
External supply  -> powered device through relay/MOSFET output side
```

If the driver module uses a separate external supply, make sure the Pico and driver input side share a common ground unless the module is optically isolated and documented otherwise.

### Raspberry Pi Whole-Room Reset Button

Wire the whole-room reset button to the Raspberry Pi:

```text
Raspberry Pi BCM GPIO 23 / physical pin 16 -> reset button -> GND
```

Convenient Raspberry Pi GND pins include physical pin 6, 9, 14, 20, 25, 30, 34, or 39.

The controller uses an internal pull-up through `libgpiod`, so no external resistor is required for the Pi reset button. Hold the button for one second to publish:

```text
escape/game/reset
```

### Pico 1: `escape-room-pico`

Stair / under-stair / entry zone.

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> copper solved input
GPIO 17 -> cubby addressable LED strip data input
GPIO 18 -> cubby 2 lock relay driver IN
GPIO 19 -> optional copper begun input
```

Stairs laser distance sensor:

```text
Sensor VCC   -> Pico 3.3V, unless the exact sensor board requires 5V
Sensor GND   -> Pico GND
Sensor SDA   -> Pico I2C SDA, usually GPIO 4
Sensor SCL   -> Pico I2C SCL, usually GPIO 5
Sensor XSHUT -> GPIO 6
Sensor GPIO1 -> GPIO 7, wired for future interrupt use but not required by current code
```

Cubby addressable LED strip:

```text
5V 3A supply + -> LED strip 5V
5V 3A supply - -> LED strip GND
5V 3A supply - -> Pico GND
Pico GPIO 17   -> LED strip DIN, preferably through a 330-470 ohm resistor
```

Recommended LED strip protection:

```text
1000uF capacitor across LED strip 5V and GND near the strip input
Keep brightness low at first
Do not power the LED strip from the Pico
```

Power budget behavior:

```text
5V 3A adapter = 3000mA available
500mA reserved headroom
2500mA default LED budget
Pico automatically dims the strip if the requested cubby colors exceed the budget
```

MQTT:

```text
Publishes:  escape/puzzle/copper/begun
Publishes:  escape/puzzle/copper/solved
Publishes:  escape/puzzle/stairs/triggered
Subscribes: escape/cubby/1/light_on
Subscribes: escape/cubby/2/light_on
Subscribes: escape/cubby/3/light_on
Subscribes: escape/cubby/4/light_on
Subscribes: escape/cubby/5/light_on
Subscribes: escape/cubby/6/light_on
Subscribes: escape/cubby/1/status
Subscribes: escape/cubby/2/status
Subscribes: escape/cubby/3/status
Subscribes: escape/cubby/4/status
Subscribes: escape/cubby/5/status
Subscribes: escape/cubby/6/status
Subscribes: escape/cubby/all/status
Subscribes: escape/cubby/2/unlock
Subscribes: escape/post/query
Subscribes: escape/game/reset
Publishes:  escape/post/cubby/1/state
```

### Pico 2: `pico-cabinet-dowels-wine`

Left wall shelf / cabinet zone.

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> dowels puzzle input
GPIO 16 -> wine puzzle input
```

MQTT:

```text
Publishes:  escape/puzzle/dowels/solved
Publishes:  escape/puzzle/wine/solved
Publishes:  escape/post/cubby/2/state
Publishes:  escape/post/cubby/3/state
Subscribes: escape/post/query
Subscribes: escape/game/reset
```

### Pico 3: `pico-fireplace-reveal-effects`

Fireplace / reveal zone.

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> PDLC smart film relay/driver IN
GPIO 16 -> smoke burst relay/driver IN
GPIO 17 -> fireplace puzzle input
GPIO 18 -> electric lock relay/driver IN
GPIO 19 -> oven rotary encoder CLK
GPIO 20 -> oven rotary encoder DT
GPIO 21 -> oven rotary encoder SW, optional reset/reference button
```

Oven dial notes:

```text
Encoder VCC -> Pico 3.3V
Encoder GND -> Pico GND
Encoder CLK -> GPIO 19
Encoder DT  -> GPIO 20
Encoder SW  -> GPIO 21
```

The oven dial is enabled only after every other puzzle has reported solved. At that point the Raspberry Pi sends `escape/lock/trigger` to unlock the electromagnetic lock and `escape/oven/enable` to allow the fireplace Pico to start checking encoder position. The current code assumes the knob starts at the pretend oven's 0/off position when enabled and counts 10-degree steps to the target:

```cpp
constexpr int OVEN_TARGET_DEGREES = 350;
constexpr int OVEN_DEGREES_PER_STEP = 10;
constexpr int OVEN_TOLERANCE_DEGREES = 5;
constexpr unsigned long OVEN_LOCK_RELEASE_MS = 100;
```

The fireplace Pico also publishes the current pretend oven temperature as degrees on `escape/oven/degrees` when the dial is enabled and whenever the encoder moves. The Raspberry Pi subscribes to this topic and records the latest value so it can later drive a display, LED bar, or other feedback prop without changing the dial puzzle.

MQTT:

```text
Publishes:  escape/puzzle/fireplace/solved
Publishes:  escape/puzzle/oven/solved
Publishes:  escape/oven/degrees
Publishes:  escape/post/cubby/4/state
Subscribes: escape/pdlc/on
Subscribes: escape/smoke/burst
Subscribes: escape/lock/trigger
Subscribes: escape/oven/enable
Subscribes: escape/post/query
Subscribes: escape/game/reset
```

### Pico 4: `pico-tv-wall`

TV wall zone.

```text
GPIO 15 -> TV intro trigger relay/driver/input interface
```

MQTT:

```text
Subscribes: escape/tv/play_intro
Subscribes: escape/game/reset
```

### Pico 5: `pico-phone-window-props`

Right wall / window prop zone.

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> phone puzzle input
GPIO 16 -> right-wall/window prop input
```

MQTT:

```text
Publishes:  escape/puzzle/phone/solved
Publishes:  escape/puzzle/window/triggered
Publishes:  escape/post/cubby/5/state
Subscribes: escape/post/query
Subscribes: escape/game/reset
```

### Pico 6: `pico-back-room-blender-final`

Back room / final zone.

```text
GPIO 14 -> local reset button -> GND
GPIO 15 -> blender flour puzzle input
GPIO 16 -> final win-condition relay/driver IN
```

MQTT:

```text
Publishes:  escape/puzzle/blender/solved
Publishes:  escape/post/cubby/6/state
Subscribes: escape/game/win
Subscribes: escape/post/query
Subscribes: escape/game/reset
```

### Power Notes

Use the Pico 3.3V pin only for small sensors and logic inputs. Use a separate power supply for locks, LED strips, smart film, smoke devices, motors, or anything with meaningful current draw.

For player-facing wiring, prefer low voltage:

```text
3.3V for Pico logic
5V for compatible relay/driver logic
12V for LED strips or locks only through proper driver hardware
```

Do not switch mains voltage unless someone with appropriate electrical experience handles that part of the build.

---

## Pico Project Setup

Open this folder in VS Code:

```text
EscapeRoom/escape-room-pico
```

Make sure the source file is located here:

```text
escape-room-pico/src/main.cpp
```

Update these values in `platformio.ini`:

```ini
-D WIFI_SSID=\"YOUR_WIFI_NAME\"
-D WIFI_PASS=\"YOUR_WIFI_PASSWORD\"
-D MQTT_BROKER=\"192.168.1.42\"
```

`MQTT_BROKER` should be the Raspberry Pi's IP address.

Build/upload through PlatformIO.

To upload to the Pico WH:

1. Unplug the Pico WH.
2. Hold the `BOOTSEL` button.
3. Plug the Pico into USB.
4. Release `BOOTSEL`.
5. Click **Upload** in PlatformIO.

---

## Raspberry Pi Controller Setup

Open this folder in VS Code on the Raspberry Pi, or use VS Code Remote SSH:

```text
EscapeRoom/raspberry-pi-controller
```

Make sure the source file is located here:

```text
raspberry-pi-controller/src/main.cpp
```

Install required Raspberry Pi packages:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev libgpiod-dev
```

Enable and start Mosquitto:

```bash
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

Check Mosquitto status:

```bash
sudo systemctl status mosquitto
```

Press `q` to exit the status view.

Build the controller:

```bash
pio run
```

Run the controller:

```bash
pio run -t exec
```

---

## Raspberry Pi Audio File

The Raspberry Pi controller currently expects the crash sound here:

```text
~/escape-room/crash.wav
```

Create the folder:

```bash
mkdir -p ~/escape-room
```

Place a `.wav` file at:

```text
/home/pi/escape-room/crash.wav
```

If the Raspberry Pi username is not `pi`, the program uses the current user's home directory automatically.

Test audio manually:

```bash
aplay ~/escape-room/crash.wav
```

---

## Manual MQTT Test

On the Raspberry Pi, run the controller:

```bash
pio run -t exec
```

In another terminal, publish a test event:

```bash
mosquitto_pub -h localhost -t escape/puzzle/copper/solved -m "manual test"
```

Expected result:

```text
MQTT message received.
Topic: escape/puzzle/copper/solved
Payload: manual test
Puzzle solved event received!
```

If the audio file exists, the Raspberry Pi should attempt to play it.

---

## First Prototype Test Plan

### Host-side code audit

Before uploading firmware, run the host-side tests from the repository root:

```powershell
.\tools\run-host-tests.ps1
```

This checks the Raspberry Pi controller routing, POST red/green logic, cubby LED math, debounce timing, and relay pulse timing without requiring Pico hardware.

### Step 1: Test Pico upload

Upload a simple blink program to the Pico WH.

Expected result:

```text
The onboard LED blinks.
```

---

### Step 2: Test copper input

Upload the copper puzzle test program.

Bridge the copper pads.

Expected result:

```text
The onboard LED turns on.
Serial Monitor prints: Puzzle solved!
```

---

### Step 3: Test MQTT manually

On the Raspberry Pi, run:

```bash
mosquitto_pub -h localhost -t escape/puzzle/copper/solved -m "manual test"
```

Expected result:

```text
The Raspberry Pi controller receives the message.
```

---

### Step 4: Test full Pico-to-Pi flow

Run the Raspberry Pi controller.

Power the Pico WH.

Bridge the copper puzzle pads.

Expected result:

```text
Pico detects puzzle solved.
Pico publishes MQTT message.
Raspberry Pi receives the event.
Raspberry Pi plays sound or triggers effect.
```

---

## Current MQTT Topics

Puzzle event topics:

```text
escape/puzzle/copper/begun
escape/puzzle/copper/solved
escape/puzzle/stairs/triggered
escape/puzzle/dowels/solved
escape/puzzle/wine/solved
escape/puzzle/blender/solved
escape/puzzle/fireplace/solved
escape/puzzle/oven/solved
escape/puzzle/phone/solved
escape/puzzle/window/triggered
```

Cubby light and POST topics:

```text
escape/cubby/1/light_on
escape/cubby/2/unlock
escape/cubby/2/light_on
escape/cubby/3/light_on
escape/cubby/4/light_on
escape/cubby/5/light_on
escape/cubby/6/light_on
escape/cubby/1/status
escape/cubby/2/status
escape/cubby/3/status
escape/cubby/4/status
escape/cubby/5/status
escape/cubby/6/status
escape/cubby/all/status
escape/post/query
escape/post/cubby/1/state
escape/post/cubby/2/state
escape/post/cubby/3/state
escape/post/cubby/4/state
escape/post/cubby/5/state
escape/post/cubby/6/state
```

Effect, prop, and whole-room command topics:

```text
escape/game/reset
escape/game/win
escape/pdlc/on
escape/lock/trigger
escape/oven/enable
escape/smoke/burst
escape/tv/play_intro
```

Telemetry topics:

```text
escape/oven/degrees
```

---

## Shared Host-Side Logic

Reusable logic that can be compiled on the desktop lives in `shared/`:

```text
EncoderDial.h       Oven dial degree normalization, wraparound, and target tolerance checks
LedPowerBudget.h    WS2812-style current estimates and automatic brightness capping math
PostState.h         POST cubby state topic/payload helpers
PulseTimer.h        Relay/output pulse timing helper
PuzzleDebounce.h    Debounced digital input helper
```

Host-side tests live in `tests/` and can be run without Pico or Raspberry Pi hardware:

```powershell
.\tools\run-host-tests.ps1
```

---

## Safety Notes

Keep player-facing electronics low voltage whenever possible.

Recommended voltages:

```text
3.3V
5V
12V
```

Do not directly power locks, smart film, smoke machines, motors, or LED strips from a Pico or Raspberry Pi GPIO pin.

Use proper relay modules, MOSFET modules, or isolated driver circuits for higher-current devices.

Every electronic lock should have a physical backup key or manual override.

Do not switch mains power unless handled by someone with appropriate electrical experience.

---

## Development Notes

This repo intentionally keeps the Pico firmware and Raspberry Pi controller as separate PlatformIO projects.

Reason:

```text
The Pico WH runs firmware.
The Raspberry Pi runs a normal Linux program.
```

They are part of the same escape-room system, but they build and run differently.

For simplicity, open each PlatformIO project folder separately in VS Code:

```text
Open this for Pico work:
EscapeRoom/escape-room-pico
EscapeRoom/pico-cabinet-dowels-wine
EscapeRoom/pico-fireplace-reveal-effects
EscapeRoom/pico-tv-wall
EscapeRoom/pico-phone-window-props
EscapeRoom/pico-back-room-blender-final

Open this for Raspberry Pi controller work:
EscapeRoom/raspberry-pi-controller
```

---

## Do Not Commit Secrets

Do not commit real WiFi passwords.

Keep committed values as placeholders:

```ini
-D WIFI_SSID=\"YOUR_WIFI_NAME\"
-D WIFI_PASS=\"YOUR_WIFI_PASSWORD\"
```

If real credentials are needed later, use a local ignored config file.

---

## Project Goal

The goal is not to build the entire escape room at once.

The first goal is one reliable working loop:

```text
Puzzle input
→ Pico detects solved state
→ Pico publishes MQTT event
→ Raspberry Pi receives event
→ Raspberry Pi triggers effect
```

Once that works, every additional escape-room puzzle is just another version of the same pattern.

Build one puzzle.
Test it.
Then add the next one.
