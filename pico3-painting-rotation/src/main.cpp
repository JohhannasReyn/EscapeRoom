#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../../shared/EscapeRoomProtocol.h"
#include "../../shared/PostState.h"

#ifndef WIFI_SSID
#define WIFI_SSID "EscapeRoom"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "BakeAt350"
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER "10.42.0.1"
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "pico3-painting-rotation"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int PAINTING_SENSOR_PIN = 15;
constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long SENSOR_TELEMETRY_MS = 1000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool paintingEnabled = false;
bool paintingTriggered = false;
int paintingLastState = LOW;
unsigned long paintingStableStart = 0;
unsigned long lastSensorTelemetry = 0;
unsigned long paintingTriggerCount = 0;

void publishPostState();

void resetPainting() {
    paintingEnabled = false;
    paintingTriggered = false;
    paintingTriggerCount = 0;
    paintingLastState = digitalRead(PAINTING_SENSOR_PIN);
    paintingStableStart = millis();
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

    if (topicText == EscapeTopic::ENABLE_PAINTING_ROTATION) {
        paintingEnabled = message != "off";
        paintingTriggered = false;
        paintingLastState = digitalRead(PAINTING_SENSOR_PIN);
        paintingStableStart = millis();
    } else if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
    } else if (topicText == EscapeTopic::RESET_PUZZLE || topicText == EscapeTopic::LEGACY_GAME_RESET) {
        resetPainting();
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

void connectMQTT() {
    mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(EscapeTopic::ENABLE_PAINTING_ROTATION);
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            mqtt.subscribe(EscapeTopic::RESET_PUZZLE);
            mqtt.subscribe(EscapeTopic::LEGACY_POST_QUERY);
            mqtt.subscribe(EscapeTopic::LEGACY_GAME_RESET);
            blink(3);
            publishPostState();
            return;
        }

        delay(MQTT_RETRY_MS);
    }
}

void publishEvent(const char* topic, const char* payload) {
    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void publishPostState() {
    publishEvent(postStateTopic(3).c_str(), postStatePayload(digitalRead(PAINTING_SENSOR_PIN) == HIGH));
}

void publishSensorTelemetry() {
    if (!mqtt.connected() || millis() - lastSensorTelemetry < SENSOR_TELEMETRY_MS) {
        return;
    }

    lastSensorTelemetry = millis();
    String payload = "painting_sensor=" + String(digitalRead(PAINTING_SENSOR_PIN));
    payload += ",enabled=" + String(paintingEnabled ? 1 : 0);
    payload += ",triggered=" + String(paintingTriggered ? 1 : 0);
    payload += ",trigger_count=" + String(paintingTriggerCount);
    mqtt.publish("escape/telemetry/pico3/painting_sensor", payload.c_str());
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);
    pinMode(PAINTING_SENSOR_PIN, INPUT);
    paintingLastState = digitalRead(PAINTING_SENSOR_PIN);
    paintingStableStart = millis();

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
        resetPainting();
        delay(500);
    }

    int state = digitalRead(PAINTING_SENSOR_PIN);
    unsigned long now = millis();

    if (state != paintingLastState) {
        paintingLastState = state;
        paintingStableStart = now;
    }

    if (paintingEnabled && state == LOW && paintingTriggered && now - paintingStableStart >= DEBOUNCE_MS) {
        paintingTriggered = false;
    }

    if (paintingEnabled && state == HIGH && !paintingTriggered && now - paintingStableStart >= DEBOUNCE_MS) {
        paintingTriggered = true;
        ++paintingTriggerCount;
        publishEvent(EscapeTopic::PAINTING_ROTATION_COMPLETE, "painting rotation complete");
    }

    publishSensorTelemetry();

    delay(50);
}
