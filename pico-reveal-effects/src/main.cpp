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

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "pico_reveal_effects"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int PDLC_PIN = 15;
constexpr int SMOKE_PIN = 16;
constexpr int TV_TRIGGER_PIN = 17;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long SMOKE_BURST_MS = 1200;
constexpr unsigned long TV_TRIGGER_MS = 500;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long smokeOffAt = 0;
unsigned long tvOffAt = 0;

void pulseOutput(int pin, unsigned long durationMs, unsigned long& offAt) {
    digitalWrite(pin, HIGH);
    digitalWrite(LED_PIN, HIGH);
    offAt = millis() + durationMs;
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

    if (String(topic) == "escape/pdlc/on") {
        digitalWrite(PDLC_PIN, message == "off" ? LOW : HIGH);
    } else if (String(topic) == "escape/smoke/burst") {
        pulseOutput(SMOKE_PIN, SMOKE_BURST_MS, smokeOffAt);
    } else if (String(topic) == "escape/tv/play_intro") {
        pulseOutput(TV_TRIGGER_PIN, TV_TRIGGER_MS, tvOffAt);
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
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            Serial.println("MQTT connected.");
            mqtt.subscribe("escape/pdlc/on");
            mqtt.subscribe("escape/smoke/burst");
            mqtt.subscribe("escape/tv/play_intro");
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
    pinMode(PDLC_PIN, OUTPUT);
    pinMode(SMOKE_PIN, OUTPUT);
    pinMode(TV_TRIGGER_PIN, OUTPUT);

    digitalWrite(PDLC_PIN, LOW);
    digitalWrite(SMOKE_PIN, LOW);
    digitalWrite(TV_TRIGGER_PIN, LOW);

    Serial.println("Reveal Effects Pico");

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

    unsigned long now = millis();

    if (smokeOffAt != 0 && now >= smokeOffAt) {
        digitalWrite(SMOKE_PIN, LOW);
        smokeOffAt = 0;
    }

    if (tvOffAt != 0 && now >= tvOffAt) {
        digitalWrite(TV_TRIGGER_PIN, LOW);
        tvOffAt = 0;
    }

    if (smokeOffAt == 0 && tvOffAt == 0) {
        digitalWrite(LED_PIN, digitalRead(PDLC_PIN));
    }
}
