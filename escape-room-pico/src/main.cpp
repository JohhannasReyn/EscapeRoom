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
#define MQTT_CLIENT_ID "pico_entry_copper_stairs"
#endif

constexpr int LED_PIN = LED_BUILTIN;

struct DigitalPuzzle {
    const char* name;
    const char* topic;
    const char* payload;
    int pin;
    bool solved;
    int lastState;
    unsigned long stableStart;
};

constexpr int COPPER_PIN = 15;
constexpr int STAIRS_PIN = 16;
constexpr int RST_PIN = 14;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr int PUZZLE_COUNT = 2;

DigitalPuzzle puzzles[PUZZLE_COUNT] = {
    {"Copper puzzle", "escape/puzzle/copper/solved", "copper puzzle solved", COPPER_PIN, false, LOW, 0},
    {"Stairs trigger", "escape/puzzle/stairs/triggered", "stairs triggered", STAIRS_PIN, false, LOW, 0},
};

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

void publishSolved(const DigitalPuzzle& puzzle) {
    Serial.print("Publishing event: ");
    Serial.print(puzzle.topic);
    Serial.print(" -> ");
    Serial.println(puzzle.payload);

    bool ok = mqtt.publish(puzzle.topic, puzzle.payload);

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
    pinMode(RST_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("Escape Room Entry Pico Controller");
    Serial.println("---------------------------------");

    connectWiFi();
    connectMQTT();

    for (int i = 0; i < PUZZLE_COUNT; ++i) {
        pinMode(puzzles[i].pin, INPUT);
        puzzles[i].lastState = digitalRead(puzzles[i].pin);
        puzzles[i].stableStart = millis();

        Serial.print("Watching ");
        Serial.print(puzzles[i].name);
        Serial.print(" on GPIO ");
        Serial.println(puzzles[i].pin);
    }

    Serial.println("Waiting for entry puzzle events...");
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
        for (int i = 0; i < PUZZLE_COUNT; ++i) {
            puzzles[i].solved = false;
            puzzles[i].lastState = digitalRead(puzzles[i].pin);
            puzzles[i].stableStart = millis();
        }

        digitalWrite(LED_PIN, LOW);
        Serial.println("Entry Pico puzzles reset.");
        delay(500);
    }

    unsigned long now = millis();

    for (int i = 0; i < PUZZLE_COUNT; ++i) {
        int state = digitalRead(puzzles[i].pin);

        if (state != puzzles[i].lastState) {
            puzzles[i].lastState = state;
            puzzles[i].stableStart = now;
        }

        unsigned long stableMs = now - puzzles[i].stableStart;

        if (state == HIGH && !puzzles[i].solved && stableMs >= DEBOUNCE_MS) {
            puzzles[i].solved = true;
            Serial.print(puzzles[i].name);
            Serial.println(" solved!");
            publishSolved(puzzles[i]);
        }
    }

    delay(50);
}
