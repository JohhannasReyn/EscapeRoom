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
#define MQTT_CLIENT_ID "pico_cabinet_dowels_wine"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;

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
    {"Dowels puzzle", "escape/puzzle/dowels/solved", "dowels puzzle solved", 15, false, LOW, 0},
    {"Wine puzzle", "escape/puzzle/wine/solved", "wine puzzle solved", 16, false, LOW, 0},
};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

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
        delay(MQTT_RETRY_MS);
    }
}

void publishSolved(const DigitalPuzzle& puzzle) {
    Serial.print("Publishing event: ");
    Serial.println(puzzle.topic);

    if (!mqtt.publish(puzzle.topic, puzzle.payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);

    Serial.println("Cabinet Dowels/Wine Pico Controller");

    connectWiFi();
    connectMQTT();

    for (DigitalPuzzle& puzzle : puzzles) {
        pinMode(puzzle.pin, INPUT);
        puzzle.lastState = digitalRead(puzzle.pin);
        puzzle.stableStart = millis();
    }
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
        for (DigitalPuzzle& puzzle : puzzles) {
            puzzle.solved = false;
            puzzle.lastState = digitalRead(puzzle.pin);
            puzzle.stableStart = millis();
        }
        digitalWrite(LED_PIN, LOW);
        Serial.println("Cabinet puzzles reset.");
        delay(500);
    }

    unsigned long now = millis();

    for (DigitalPuzzle& puzzle : puzzles) {
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
