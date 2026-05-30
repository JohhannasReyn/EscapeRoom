#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

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
#define MQTT_CLIENT_ID "pico_tv_wall"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int TV_TRIGGER_PIN = 15;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long TV_TRIGGER_MS = 500;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long tvOffAt = 0;

void pulseTvTrigger() {
    digitalWrite(TV_TRIGGER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    tvOffAt = millis() + TV_TRIGGER_MS;
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

    if (String(topic) == "escape/game/reset") {
        tvOffAt = 0;
        digitalWrite(TV_TRIGGER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        Serial.println("TV wall reset.");
    } else if (String(topic) == "escape/tv/play_intro") {
        pulseTvTrigger();
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
            mqtt.subscribe("escape/tv/play_intro");
            mqtt.subscribe("escape/game/reset");
            blink(3);
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        delay(MQTT_RETRY_MS);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(TV_TRIGGER_PIN, OUTPUT);

    digitalWrite(TV_TRIGGER_PIN, LOW);

    Serial.println("TV Wall Pico");

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

    if (tvOffAt != 0 && millis() >= tvOffAt) {
        digitalWrite(TV_TRIGGER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        tvOffAt = 0;
    }
}
