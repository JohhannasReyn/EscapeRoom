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
#define MQTT_CLIENT_ID "pico_copper_puzzle"
#endif

#ifndef MQTT_TOPIC_SOLVED
#define MQTT_TOPIC_SOLVED "escape/puzzle/copper/solved"
#endif

constexpr int LED_PIN = LED_BUILTIN;

// Copper puzzle input.
// 3.3V -> Copper Pad A
// Copper Pad B -> GPIO 15
// GPIO 15 -> 10k resistor -> GND
constexpr int PUZ_PIN = 15;

// Optional reset button.
// Button from GPIO 14 to GND.
// Uses internal pull-up.
constexpr int RST_PIN = 14;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool solved = false;
int lastState = LOW;
unsigned long stableStart = 0;

constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;

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
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            Serial.println("MQTT connected.");
            blink(3);
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        Serial.println("Retrying...");
        delay(MQTT_RETRY_MS);
    }
}

void publishSolved() {
    const char* msg = "copper puzzle solved";

    Serial.print("Publishing solved event: ");
    Serial.println(msg);

    bool ok = mqtt.publish(MQTT_TOPIC_SOLVED, msg);

    if (ok) {
        Serial.println("MQTT publish successful.");
    } else {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(PUZ_PIN, INPUT);
    pinMode(RST_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("Escape Room Copper Puzzle Controller");
    Serial.println("------------------------------------");

    connectWiFi();
    connectMQTT();

    lastState = digitalRead(PUZ_PIN);
    stableStart = millis();

    Serial.println("Waiting for copper puzzle contact...");
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
        solved = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("Puzzle reset.");
        delay(500);
    }

    int state = digitalRead(PUZ_PIN);
    unsigned long now = millis();

    if (state != lastState) {
        lastState = state;
        stableStart = now;
    }

    unsigned long stableMs = now - stableStart;

    if (state == HIGH && !solved && stableMs >= DEBOUNCE_MS) {
        solved = true;
        Serial.println("Puzzle solved!");
        publishSolved();
    }

    delay(50);
}