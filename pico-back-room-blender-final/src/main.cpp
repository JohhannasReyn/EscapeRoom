#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

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
#define MQTT_CLIENT_ID "pico_back_room_blender_final"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int BLENDER_PIN = 15;
constexpr int FINAL_OUTPUT_PIN = 16;
constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool blenderSolved = false;
int blenderLastState = LOW;
unsigned long blenderStableStart = 0;

void resetBlenderAndOutputs();
void publishPostState();

void setFinalOutput(bool on) {
    digitalWrite(FINAL_OUTPUT_PIN, on ? HIGH : LOW);
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    Serial.print("Command received: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(message);

    if (String(topic) == "escape/post/query") {
        publishPostState();
    } else if (String(topic) == "escape/game/reset") {
        resetBlenderAndOutputs();
    } else if (String(topic) == "escape/game/win") {
        setFinalOutput(message != "off");
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
            Serial.println("MQTT connected.");
            mqtt.subscribe("escape/game/win");
            mqtt.subscribe("escape/post/query");
            mqtt.subscribe("escape/game/reset");
            blink(3);
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        delay(MQTT_RETRY_MS);
    }
}

void publishBlenderSolved() {
    const char* topic = "escape/puzzle/blender/solved";
    const char* payload = "blender puzzle solved";

    Serial.print("Publishing event: ");
    Serial.println(topic);

    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void publishPostState() {
    std::string topic = postStateTopic(6);
    const char* payload = postStatePayload(digitalRead(BLENDER_PIN) == HIGH);

    Serial.print("Publishing POST state: ");
    Serial.print(topic.c_str());
    Serial.print(" -> ");
    Serial.println(payload);

    if (!mqtt.publish(topic.c_str(), payload)) {
        Serial.println("MQTT publish failed.");
    }
}

void resetBlenderAndOutputs() {
    blenderSolved = false;
    blenderLastState = digitalRead(BLENDER_PIN);
    blenderStableStart = millis();
    setFinalOutput(false);
    digitalWrite(LED_PIN, LOW);
    if (mqtt.connected()) {
        publishPostState();
    }
    Serial.println("Back room blender/final zone reset.");
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);
    pinMode(BLENDER_PIN, INPUT);
    pinMode(FINAL_OUTPUT_PIN, OUTPUT);

    digitalWrite(FINAL_OUTPUT_PIN, LOW);

    Serial.println("Back Room Blender/Final Pico");

    blenderLastState = digitalRead(BLENDER_PIN);
    blenderStableStart = millis();

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
        resetBlenderAndOutputs();
        delay(500);
    }

    int state = digitalRead(BLENDER_PIN);
    unsigned long now = millis();

    if (state != blenderLastState) {
        blenderLastState = state;
        blenderStableStart = now;
    }

    if (state == HIGH && !blenderSolved && now - blenderStableStart >= DEBOUNCE_MS) {
        blenderSolved = true;
        Serial.println("Blender puzzle solved!");
        publishBlenderSolved();
    }

    delay(50);
}
