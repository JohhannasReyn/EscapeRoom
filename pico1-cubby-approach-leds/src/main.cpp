#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <VL53L0X.h>

#include "CubbyLedLayout.h"
#include "../../shared/EscapeRoomProtocol.h"
#include "../../shared/LedPowerBudget.h"
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
#define MQTT_CLIENT_ID "pico1-cubby-approach-leds"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int CUBBY_LED_DATA_PIN = 17;
constexpr int DISTANCE_SENSOR_XSHUT_PIN = 6;
constexpr int DISTANCE_SENSOR_GPIO1_PIN = 7;

constexpr int CUBBY_COUNT = 6;
constexpr int LEDS_PER_CUBBY = 30;
constexpr int LEDS_BETWEEN_CUBBIES = 3;
constexpr int CUBBY_LED_BRIGHTNESS = 80;
constexpr int CUBBY_ACTIVE_DISTANCE_MM = 650;
constexpr int STARTUP_CUBBY_STEP_MS = 300;
constexpr int STARTUP_ALL_WHITE_MS = 800;
constexpr int LED_SUPPLY_MA = 3000;
constexpr int LED_POWER_HEADROOM_MA = 500;
constexpr int LED_CURRENT_BUDGET_MA = LED_SUPPLY_MA - LED_POWER_HEADROOM_MA;
constexpr int LED_FULL_WHITE_CURRENT_MA = 60;

constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long DISTANCE_READ_MS = 100;

constexpr int TOTAL_CUBBY_LEDS = totalCubbyLedCount(CUBBY_COUNT, LEDS_PER_CUBBY, LEDS_BETWEEN_CUBBIES);

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_NeoPixel cubbyStrip(TOTAL_CUBBY_LEDS, CUBBY_LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
VL53L0X distanceSensor;

bool distanceSensorReady = false;
bool approachTriggered = false;
bool readyForGameplay = false;
unsigned long lastDistanceRead = 0;
RgbColor cubbyColors[CUBBY_COUNT] = {};

void resetController(bool readyAfterReset = true);
void publishPostState();

uint32_t toStripColor(RgbColor color) {
    return cubbyStrip.Color(color.r, color.g, color.b);
}

int estimateCubbyStripCurrentMa() {
    int estimatedCurrent = 0;

    for (int cubbyNumber = 1; cubbyNumber <= CUBBY_COUNT; ++cubbyNumber) {
        estimatedCurrent += estimateSegmentCurrentMa(
            cubbyColors[cubbyNumber - 1],
            LEDS_PER_CUBBY,
            CUBBY_LED_BRIGHTNESS,
            LED_FULL_WHITE_CURRENT_MA
        );
    }

    return estimatedCurrent;
}

void applyCubbyLights() {
    int estimatedCurrent = estimateCubbyStripCurrentMa();
    int cappedBrightness = cappedBrightnessForBudget(CUBBY_LED_BRIGHTNESS, estimatedCurrent, LED_CURRENT_BUDGET_MA);
    cubbyStrip.setBrightness(cappedBrightness);

    for (int cubbyNumber = 1; cubbyNumber <= CUBBY_COUNT; ++cubbyNumber) {
        CubbyLedSegment segment = cubbySegment(cubbyNumber, LEDS_PER_CUBBY, LEDS_BETWEEN_CUBBIES);
        uint32_t stripColor = toStripColor(cubbyColors[cubbyNumber - 1]);

        for (int offset = 0; offset < segment.count; ++offset) {
            cubbyStrip.setPixelColor(segment.start + offset, stripColor);
        }
    }

    cubbyStrip.show();
}

void clearCubbyLights() {
    for (int cubbyIndex = 0; cubbyIndex < CUBBY_COUNT; ++cubbyIndex) {
        cubbyColors[cubbyIndex] = {0, 0, 0};
    }

    applyCubbyLights();
}

void setCubbySegmentColor(int cubbyNumber, RgbColor color) {
    CubbyLedSegment segment = cubbySegment(cubbyNumber, LEDS_PER_CUBBY, LEDS_BETWEEN_CUBBIES);

    if (cubbyNumber < 1 || cubbyNumber > CUBBY_COUNT || segment.count == 0) {
        mqtt.publish(EscapeTopic::CUBBY_LED_ERROR, "invalid cubby number");
        return;
    }

    cubbyColors[cubbyNumber - 1] = color;
    applyCubbyLights();
    mqtt.publish(EscapeTopic::CUBBY_LED_READY, "ready");
}

void setAllCubbySegments(RgbColor color) {
    for (int cubbyNumber = 1; cubbyNumber <= CUBBY_COUNT; ++cubbyNumber) {
        cubbyColors[cubbyNumber - 1] = color;
    }

    applyCubbyLights();
}

void setCubbyPostStatus(int cubbyNumber, const String& status) {
    if (status == "green") {
        setCubbySegmentColor(cubbyNumber, {0, 255, 0});
    } else if (status == "red") {
        setCubbySegmentColor(cubbyNumber, {255, 0, 0});
    } else if (status == "white") {
        setCubbySegmentColor(cubbyNumber, {255, 255, 255});
    } else if (status == "off") {
        setCubbySegmentColor(cubbyNumber, {0, 0, 0});
    }
}

void runStartupLightPost() {
    clearCubbyLights();

    for (int cubbyNumber = 1; cubbyNumber <= CUBBY_COUNT; ++cubbyNumber) {
        setCubbySegmentColor(cubbyNumber, {255, 210, 120});
        delay(STARTUP_CUBBY_STEP_MS);
    }

    setAllCubbySegments({255, 255, 255});
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
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    String topicText(topic);
    Serial.print("Command received: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(message);

    if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
    } else if (topicText == EscapeTopic::RESET_PUZZLE || topicText == EscapeTopic::LEGACY_GAME_RESET) {
        resetController(true);
    } else if (topicText == EscapeTopic::ENABLE_CUBBY_LIGHT || topicText == EscapeTopic::LEGACY_CUBBY_1_LIGHT_ON) {
        setCubbySegmentColor(message.toInt() > 0 ? message.toInt() : 1, {255, 210, 120});
    } else if (topicText == "escape/cubby/all/status" && message == "off") {
        readyForGameplay = true;
        clearCubbyLights();
    } else if (topicText.startsWith("escape/cubby/") && topicText.endsWith("/status")) {
        int cubbyStart = String("escape/cubby/").length();
        int cubbyEnd = topicText.indexOf("/status");
        setCubbyPostStatus(topicText.substring(cubbyStart, cubbyEnd).toInt(), message);
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

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
        Serial.print(".");
    }

    digitalWrite(LED_PIN, LOW);
    Serial.println();
    Serial.println("WiFi connected.");
    blink(2);
}

void connectMQTT() {
    mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(EscapeTopic::ENABLE_CUBBY_LIGHT);
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            mqtt.subscribe(EscapeTopic::RESET_PUZZLE);
            mqtt.subscribe(EscapeTopic::LEGACY_CUBBY_1_LIGHT_ON);
            mqtt.subscribe(EscapeTopic::LEGACY_POST_QUERY);
            mqtt.subscribe(EscapeTopic::LEGACY_GAME_RESET);
            mqtt.subscribe("escape/cubby/1/status");
            mqtt.subscribe("escape/cubby/2/status");
            mqtt.subscribe("escape/cubby/3/status");
            mqtt.subscribe("escape/cubby/4/status");
            mqtt.subscribe("escape/cubby/5/status");
            mqtt.subscribe("escape/cubby/all/status");
            blink(3);
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        delay(MQTT_RETRY_MS);
    }
}

void publishPostState() {
    publishEvent(postStateTopic(1).c_str(), postStatePayload(false));
}

void resetController(bool readyAfterReset) {
    approachTriggered = false;
    readyForGameplay = readyAfterReset;
    digitalWrite(LED_PIN, LOW);
    clearCubbyLights();
    if (mqtt.connected()) {
        publishPostState();
    }
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
        Serial.println("VL53L0X distance sensor not found; cubby approach disabled.");
        return;
    }

    distanceSensor.startContinuous();
    distanceSensorReady = true;
}

void checkCubbyApproach() {
    if (!readyForGameplay || !distanceSensorReady || approachTriggered || millis() - lastDistanceRead < DISTANCE_READ_MS) {
        return;
    }

    lastDistanceRead = millis();
    uint16_t distanceMm = distanceSensor.readRangeContinuousMillimeters();

    if (!distanceSensor.timeoutOccurred() && distanceMm <= CUBBY_ACTIVE_DISTANCE_MM) {
        approachTriggered = true;
        publishEvent(EscapeTopic::CUBBY_APPROACH_DETECTED, "approach detected");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);

    cubbyStrip.begin();
    cubbyStrip.setBrightness(CUBBY_LED_BRIGHTNESS);
    clearCubbyLights();
    runStartupLightPost();

    setupDistanceSensor();
    connectWiFi();
    connectMQTT();
    readyForGameplay = true;
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
        resetController(true);
        delay(500);
    }

    checkCubbyApproach();
    delay(50);
}
