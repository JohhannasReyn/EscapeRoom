#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <VL53L0X.h>

#include "CubbyLedLayout.h"
#include "../../shared/PostState.h"

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_NAME"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER "192.168.1.42"
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "pico_entry_stairs_cubby_copper"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int COPPER_SOLVED_PIN = 15;
constexpr int COPPER_STARTED_PIN = 19;
constexpr bool COPPER_STARTED_INPUT_ENABLED = true;
constexpr int CUBBY_LED_DATA_PIN = 17;
constexpr int CUBBY_2_UNLOCK_PIN = 18;
constexpr int DISTANCE_SENSOR_XSHUT_PIN = 6;
constexpr int DISTANCE_SENSOR_GPIO1_PIN = 7;

// Adjust these after the LED strip is installed in the cubbies.
constexpr int CUBBY_COUNT = 6;
constexpr int LEDS_PER_CUBBY = 30;
constexpr int LEDS_BETWEEN_CUBBIES = 3;
constexpr int CUBBY_LED_BRIGHTNESS = 80;
constexpr int CUBBY_ACTIVE_DISTANCE_MM = 650;
constexpr int STARTUP_CUBBY_STEP_MS = 300;
constexpr int STARTUP_ALL_WHITE_MS = 800;

constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long DISTANCE_READ_MS = 100;

constexpr int TOTAL_CUBBY_LEDS = totalCubbyLedCount(CUBBY_COUNT, LEDS_PER_CUBBY, LEDS_BETWEEN_CUBBIES);

struct DigitalPuzzle {
    const char* name;
    const char* topic;
    const char* payload;
    int pin;
    bool solved;
    int lastState;
    unsigned long stableStart;
};

DigitalPuzzle puzzles[] = {
    {"Copper puzzle solved", "escape/puzzle/copper/solved", "copper puzzle solved", COPPER_SOLVED_PIN, false, LOW, 0},
    {"Copper puzzle begun", "escape/puzzle/copper/begun", "copper puzzle begun", COPPER_STARTED_PIN, false, LOW, 0},
};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_NeoPixel cubbyStrip(TOTAL_CUBBY_LEDS, CUBBY_LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
VL53L0X distanceSensor;

bool distanceSensorReady = false;
bool stairsTriggered = false;
bool readyForGameplay = false;
unsigned long lastDistanceRead = 0;

void resetPuzzles(bool readyAfterReset = true);
void setCubbyLight(int cubbyNumber, bool on);
void publishPostState();

void setOutput(int pin, bool on) {
    digitalWrite(pin, on ? HIGH : LOW);
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void clearCubbyLights() {
    cubbyStrip.clear();
    cubbyStrip.show();
}

void setCubbySegmentColor(int cubbyNumber, uint32_t color) {
    CubbyLedSegment segment = cubbySegment(cubbyNumber, LEDS_PER_CUBBY, LEDS_BETWEEN_CUBBIES);

    if (cubbyNumber < 1 || cubbyNumber > CUBBY_COUNT || segment.count == 0) {
        Serial.print("Ignoring cubby command for cubby ");
        Serial.println(cubbyNumber);
        return;
    }

    for (int offset = 0; offset < segment.count; ++offset) {
        cubbyStrip.setPixelColor(segment.start + offset, color);
    }

    cubbyStrip.show();
}

void setAllCubbySegments(uint32_t color) {
    for (int cubbyNumber = 1; cubbyNumber <= CUBBY_COUNT; ++cubbyNumber) {
        CubbyLedSegment segment = cubbySegment(cubbyNumber, LEDS_PER_CUBBY, LEDS_BETWEEN_CUBBIES);

        for (int offset = 0; offset < segment.count; ++offset) {
            cubbyStrip.setPixelColor(segment.start + offset, color);
        }
    }

    cubbyStrip.show();
}

void setCubbyLight(int cubbyNumber, bool on) {
    setCubbySegmentColor(cubbyNumber, on ? cubbyStrip.Color(255, 210, 120) : 0);
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void setCubbyPostStatus(int cubbyNumber, const String& status) {
    if (status == "green") {
        setCubbySegmentColor(cubbyNumber, cubbyStrip.Color(0, 255, 0));
    } else if (status == "red") {
        setCubbySegmentColor(cubbyNumber, cubbyStrip.Color(255, 0, 0));
    } else if (status == "white") {
        setCubbySegmentColor(cubbyNumber, cubbyStrip.Color(255, 255, 255));
    } else if (status == "off") {
        setCubbySegmentColor(cubbyNumber, 0);
    }
}

void runStartupLightPost() {
    clearCubbyLights();

    for (int cubbyNumber = 1; cubbyNumber <= CUBBY_COUNT; ++cubbyNumber) {
        setCubbySegmentColor(cubbyNumber, cubbyStrip.Color(255, 210, 120));
        delay(STARTUP_CUBBY_STEP_MS);
    }

    setAllCubbySegments(cubbyStrip.Color(255, 255, 255));
    delay(STARTUP_ALL_WHITE_MS);
}

void publishEvent(const char* topic, const char* payload) {
    Serial.print("Publishing event: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);

    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    bool on = message != "off";

    Serial.print("Command received: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(message);

    String topicText(topic);

    if (topicText == "escape/post/query") {
        publishPostState();
    } else if (topicText == "escape/game/reset") {
        resetPuzzles(true);
    } else if (topicText == "escape/cubby/all/status") {
        if (message == "off") {
            readyForGameplay = true;
            clearCubbyLights();
            Serial.println("POST complete. Pico is ready for gameplay.");
        } else if (message == "white") {
            readyForGameplay = false;
            setAllCubbySegments(cubbyStrip.Color(255, 255, 255));
        }
    } else if (topicText.startsWith("escape/cubby/") && topicText.endsWith("/status")) {
        int cubbyStart = String("escape/cubby/").length();
        int cubbyEnd = topicText.indexOf("/status");
        int cubbyNumber = topicText.substring(cubbyStart, cubbyEnd).toInt();
        setCubbyPostStatus(cubbyNumber, message);
    } else if (topicText.startsWith("escape/cubby/") && topicText.endsWith("/light_on")) {
        int cubbyStart = String("escape/cubby/").length();
        int cubbyEnd = topicText.indexOf("/light_on");
        int cubbyNumber = topicText.substring(cubbyStart, cubbyEnd).toInt();
        setCubbyLight(cubbyNumber, on);
    } else if (topicText == "escape/cubby/2/unlock") {
        setOutput(CUBBY_2_UNLOCK_PIN, on);
    }
}

void blink(int count, int delayMs = 150) {
    for (int i = 0; i < count; ++i) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

void connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int tries = 0;

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
        Serial.print(".");

        ++tries;

        if (tries > 60) {
            Serial.println();
            Serial.println("WiFi connection failed. Retrying...");
            tries = 0;
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
    }

    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("WiFi connected.");
    Serial.print("Pico IP address: ");
    Serial.println(WiFi.localIP());

    blink(2);
}

void connectMQTT() {
    mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            Serial.println("MQTT connected.");
            mqtt.subscribe("escape/cubby/1/light_on");
            mqtt.subscribe("escape/cubby/2/light_on");
            mqtt.subscribe("escape/cubby/3/light_on");
            mqtt.subscribe("escape/cubby/4/light_on");
            mqtt.subscribe("escape/cubby/5/light_on");
            mqtt.subscribe("escape/cubby/6/light_on");
            mqtt.subscribe("escape/cubby/1/status");
            mqtt.subscribe("escape/cubby/2/status");
            mqtt.subscribe("escape/cubby/3/status");
            mqtt.subscribe("escape/cubby/4/status");
            mqtt.subscribe("escape/cubby/5/status");
            mqtt.subscribe("escape/cubby/6/status");
            mqtt.subscribe("escape/cubby/all/status");
            mqtt.subscribe("escape/cubby/2/unlock");
            mqtt.subscribe("escape/post/query");
            mqtt.subscribe("escape/game/reset");
            blink(3);
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        Serial.println("Retrying...");
        delay(MQTT_RETRY_MS);
    }
}

void publishSolved(const DigitalPuzzle& puzzle) {
    publishEvent(puzzle.topic, puzzle.payload);
}

bool copperSolvedContactIsCompleted() {
    return digitalRead(COPPER_SOLVED_PIN) == HIGH;
}

void publishPostState() {
    publishEvent(postStateTopic(1).c_str(), postStatePayload(copperSolvedContactIsCompleted()));
}

void resetPuzzles(bool readyAfterReset) {
    for (DigitalPuzzle& puzzle : puzzles) {
        if (puzzle.pin == COPPER_STARTED_PIN && !COPPER_STARTED_INPUT_ENABLED) {
            continue;
        }

        puzzle.solved = false;
        puzzle.lastState = digitalRead(puzzle.pin);
        puzzle.stableStart = millis();
    }

    stairsTriggered = false;
    readyForGameplay = readyAfterReset;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(CUBBY_2_UNLOCK_PIN, LOW);
    clearCubbyLights();
    if (mqtt.connected()) {
        publishPostState();
    }
    Serial.println("Entry/stairs/cubby Pico puzzles reset.");
}

void setupDistanceSensor() {
    pinMode(DISTANCE_SENSOR_XSHUT_PIN, OUTPUT);
    pinMode(DISTANCE_SENSOR_GPIO1_PIN, INPUT);

    digitalWrite(DISTANCE_SENSOR_XSHUT_PIN, LOW);
    delay(10);
    digitalWrite(DISTANCE_SENSOR_XSHUT_PIN, HIGH);
    delay(10);

    Wire.begin();

    distanceSensor.setTimeout(500);

    if (!distanceSensor.init()) {
        distanceSensorReady = false;
        Serial.println("VL53L0X distance sensor not found; stairs trigger disabled.");
        return;
    }

    distanceSensor.startContinuous();
    distanceSensorReady = true;
    Serial.println("VL53L0X distance sensor ready.");
}

void checkStairsDistance() {
    if (!readyForGameplay || !distanceSensorReady || stairsTriggered || millis() - lastDistanceRead < DISTANCE_READ_MS) {
        return;
    }

    lastDistanceRead = millis();

    uint16_t distanceMm = distanceSensor.readRangeContinuousMillimeters();

    if (!distanceSensor.timeoutOccurred() && distanceMm <= CUBBY_ACTIVE_DISTANCE_MM) {
        stairsTriggered = true;
        setCubbyLight(1, true);
        publishEvent("escape/puzzle/stairs/triggered", "stairs triggered");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);
    pinMode(CUBBY_2_UNLOCK_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(CUBBY_2_UNLOCK_PIN, LOW);
    cubbyStrip.begin();
    cubbyStrip.setBrightness(CUBBY_LED_BRIGHTNESS);
    clearCubbyLights();
    runStartupLightPost();

    Serial.println();
    Serial.println("Escape Room Entry/Stairs/Cubby Pico");
    Serial.println("-----------------------------------");

    for (DigitalPuzzle& puzzle : puzzles) {
        if (puzzle.pin == COPPER_STARTED_PIN && !COPPER_STARTED_INPUT_ENABLED) {
            Serial.println("Copper begun input disabled.");
            continue;
        }

        pinMode(puzzle.pin, INPUT);
        puzzle.lastState = digitalRead(puzzle.pin);
        puzzle.stableStart = millis();

        Serial.print("Watching ");
        Serial.print(puzzle.name);
        Serial.print(" on GPIO ");
        Serial.println(puzzle.pin);
    }

    setupDistanceSensor();
    connectWiFi();
    connectMQTT();

    Serial.println("Waiting for entry/stairs/cubby events...");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (!mqtt.connected()) {
        connectMQTT();
    }

    mqtt.loop();

    if (digitalRead(RST_PIN) == LOW) {
        resetPuzzles(true);
        delay(500);
    }

    unsigned long now = millis();
    checkStairsDistance();

    for (DigitalPuzzle& puzzle : puzzles) {
        if (puzzle.pin == COPPER_STARTED_PIN && !COPPER_STARTED_INPUT_ENABLED) {
            continue;
        }

        int state = digitalRead(puzzle.pin);

        if (state != puzzle.lastState) {
            puzzle.lastState = state;
            puzzle.stableStart = now;
        }

        if (state == HIGH && !puzzle.solved && now - puzzle.stableStart >= DEBOUNCE_MS) {
            puzzle.solved = true;
            Serial.print(puzzle.name);
            Serial.println(" solved!");
            publishSolved(puzzle);
        }
    }

    delay(50);
}
