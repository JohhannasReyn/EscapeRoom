#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../../shared/EscapeRoomProtocol.h"
#include "../../shared/PostState.h"

#ifndef WIFI_SSID
#define WIFI_SSID "VenueWifi"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "VenuePassword"
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER "ceenypie.local"
#endif

#ifndef MQTT_BROKER_FALLBACK
#define MQTT_BROKER_FALLBACK ""
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "pico2-copper-final-piece"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int COPPER_COMPLETE_PIN = 15;
constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long SENSOR_TELEMETRY_MS = 1000;
constexpr int MQTT_ATTEMPTS_PER_HOST = 3;
constexpr int PUZZLE_ACTIVE_STATE = LOW;
constexpr int PUZZLE_INACTIVE_STATE = HIGH;

struct DigitalPuzzle {
    const char* name;
    const char* topic;
    const char* payload;
    int pin;
    bool enabled;
    bool solved;
    int lastState;
    unsigned long stableStart;
};

DigitalPuzzle copperPuzzle = {
    "Copper puzzle",
    EscapeTopic::COPPER_PUZZLE_COMPLETE,
    "Complete",
    COPPER_COMPLETE_PIN,
    true,
    false,
    PUZZLE_INACTIVE_STATE,
    0,
};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastSensorTelemetry = 0;

void publishPostState();

void resetPuzzles() {
    copperPuzzle.solved = false;
    copperPuzzle.lastState = digitalRead(copperPuzzle.pin);
    copperPuzzle.stableStart = millis();

    digitalWrite(LED_PIN, LOW);
    if (mqtt.connected()) {
        publishPostState();
    }
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    String topicText(topic);

    if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
    } else if (topicText == EscapeTopic::RESET_PUZZLE || topicText == EscapeTopic::LEGACY_GAME_RESET) {
        resetPuzzles();
    } else if (topicText == EscapeTopic::ENABLE_COPPER_PUZZLE) {
        copperPuzzle.enabled = message != "off";
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
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
    }

    digitalWrite(LED_PIN, LOW);
    blink(2);
}

bool tryConnectMQTT(const char* broker) {
    if (broker == nullptr || broker[0] == '\0') {
        return false;
    }

    mqtt.setServer(broker, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    for (int attempt = 0; attempt < MQTT_ATTEMPTS_PER_HOST && !mqtt.connected(); ++attempt) {
        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(EscapeTopic::ENABLE_COPPER_PUZZLE);
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            mqtt.subscribe(EscapeTopic::RESET_PUZZLE);
            mqtt.subscribe(EscapeTopic::LEGACY_POST_QUERY);
            mqtt.subscribe(EscapeTopic::LEGACY_GAME_RESET);
            blink(3);
            publishPostState();
            return true;
        }

        delay(MQTT_RETRY_MS);
    }

    return false;
}

void connectMQTT() {
    while (!mqtt.connected()) {
        if (tryConnectMQTT(MQTT_BROKER) || tryConnectMQTT(MQTT_BROKER_FALLBACK)) {
            return;
        }
    }
}

void publishEvent(const char* topic, const char* payload) {
    Serial.print("Publishing event: ");
    Serial.println(topic);

    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void publishPostState() {
    bool completed = digitalRead(COPPER_COMPLETE_PIN) == PUZZLE_ACTIVE_STATE;
    publishEvent(postStateTopic(2).c_str(), postStatePayload(completed));
}

void publishSensorTelemetry() {
    if (!mqtt.connected() || millis() - lastSensorTelemetry < SENSOR_TELEMETRY_MS) {
        return;
    }

    lastSensorTelemetry = millis();
    String payload = "copper=" + String(digitalRead(COPPER_COMPLETE_PIN));
    payload += ",copper_solved=" + String(copperPuzzle.solved ? 1 : 0);
    mqtt.publish("escape/telemetry/pico2/contacts", payload.c_str());
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);

    pinMode(copperPuzzle.pin, INPUT_PULLUP);
    copperPuzzle.lastState = digitalRead(copperPuzzle.pin);
    copperPuzzle.stableStart = millis();

    connectWiFi();
    connectMQTT();
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
        resetPuzzles();
        delay(500);
    }

    unsigned long now = millis();

    int state = digitalRead(copperPuzzle.pin);

    if (state != copperPuzzle.lastState) {
        copperPuzzle.lastState = state;
        copperPuzzle.stableStart = now;
    }

    if (state == PUZZLE_INACTIVE_STATE && copperPuzzle.solved && now - copperPuzzle.stableStart >= DEBOUNCE_MS) {
        copperPuzzle.solved = false;
        digitalWrite(LED_PIN, LOW);
        publishPostState();
    }

    if (copperPuzzle.enabled && state == PUZZLE_ACTIVE_STATE && !copperPuzzle.solved && now - copperPuzzle.stableStart >= DEBOUNCE_MS) {
        copperPuzzle.solved = true;
        publishEvent(copperPuzzle.topic, copperPuzzle.payload);
    }

    publishSensorTelemetry();

    delay(50);
}
