# Escape Room Electronics Project

This repository contains the electronics and controller code for a modular escape room project.

The first working prototype is a copper/contact puzzle:

```text
Copper/contact puzzle piece
→ Pico WH detects completed circuit
→ Pico WH publishes MQTT event over WiFi
→ Raspberry Pi receives event
→ Raspberry Pi plays audio or triggers an effect
````

The long-term goal is to build a full escape room using multiple physical puzzles, sensors, lights, locks, audio cues, smart film, and themed props.
(**June 6th Deadline**)

---

## Current Project Structure

```text
EscapeRoom/
├── README.md
├── escape-room-pico/
│   ├── platformio.ini
│   └── src/
│       └── main.cpp
│
└── raspberry-pi-controller/
    ├── platformio.ini
    └── src/
        └── main.cpp
```

---

## Projects

### `escape-room-pico`

This project runs on a Raspberry Pi Pico WH near the entry puzzle area.

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
```

### Pico project grouping

There are six deployable Pico WH projects. Puzzles are grouped by physical proximity so the wiring stays local and each Pico still has GPIO headroom for reset buttons, status LEDs, and future inputs.

```text
escape-room-pico/
  Pico 1: entry area
  Inputs: copper puzzle, stairs trigger
  Publishes:
    escape/puzzle/copper/solved
    escape/puzzle/stairs/triggered

pico-cabinet-dowels-wine/
  Pico 2: cabinet/table area
  Inputs: dowels puzzle, wine puzzle
  Publishes:
    escape/puzzle/dowels/solved
    escape/puzzle/wine/solved

pico-kitchen-blender-fireplace/
  Pico 3: kitchen/hearth area
  Inputs: blender flour puzzle, fireplace log puzzle
  Publishes:
    escape/puzzle/blender/solved
    escape/puzzle/fireplace/solved

pico-phone-prop/
  Pico 4: phone prop
  Inputs: phone puzzle
  Publishes:
    escape/puzzle/phone/solved

pico-cubby-controller/
  Pico 5: cubby outputs
  Subscribes:
    escape/cubby/1/light_on
    escape/cubby/2/unlock

pico-reveal-effects/
  Pico 6: reveal/effects outputs
  Subscribes:
    escape/pdlc/on
    escape/smoke/burst
    escape/tv/play_intro
```

The two output-heavy Pico projects should use relay modules, MOSFET driver modules, or isolated driver boards. If the number of cubby lights, locks, or reveal effects grows, add an I2C GPIO expander such as an MCP23017 instead of trying to crowd every device onto direct Pico GPIO.

---

### `raspberry-pi-controller`

This project runs on the Raspberry Pi 4.

Responsibilities:

* Run the main game controller
* Listen for MQTT puzzle events
* Trigger audio, video, lights, locks, smart film, smoke effects, or other outputs
* Track game state as the room grows

Current prototype behavior:

```text
Raspberry Pi listens for:
escape/puzzle/copper/solved

When received:
→ print event info
→ play crash sound
```

Controller code layout:

```text
src/main.cpp
  Sets up MQTT and forwards incoming messages to the game controller.

src/GameController.*
  Owns puzzle modules and routes MQTT topics to the matching class.

src/puzzles/
  Contains one class per puzzle topic. CopperPuzzle is the working prototype.

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
Pico 3.3V  → Copper Pad A

Copper Pad B → GPIO 15

GPIO 15 → 10kΩ resistor → GND
```

When the copper puzzle piece bridges Copper Pad A and Copper Pad B, GPIO 15 reads HIGH.

Optional reset button:

```text
GPIO 14 → Button → GND
```

GPIO 14 uses the Pico internal pull-up resistor.

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
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev
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

```text
escape/puzzle/copper/solved
```

Planned future topics:

```text
escape/puzzle/stairs/triggered
escape/puzzle/dowels/solved
escape/puzzle/wine/solved
escape/puzzle/blender/solved
escape/puzzle/fireplace/solved
escape/puzzle/phone/solved
escape/game/win

escape/cubby/1/light_on
escape/cubby/2/unlock
escape/pdlc/on
escape/audio/crash
escape/audio/ralph_01
escape/smoke/burst
escape/tv/play_intro
```

---

## Planned Puzzle Modules

Future folders may include:

```text
pico-cabinet-dowels-wine/
pico-kitchen-blender-fireplace/
pico-phone-prop/
pico-cubby-controller/
pico-reveal-effects/
docs/
assets/
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

````

Then from PowerShell, create it quickly with:

```powershell
notepad README.md
````

Paste the content, save, then:

```powershell
git add README.md
git commit -m "Add project README"
```
