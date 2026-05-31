#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../../shared/EscapeRoomProtocol.h"
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
#define MQTT_CLIENT_ID "pico5-color-buttons"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int BUTTON_RED_PIN = 15;
constexpr int BUTTON_BLUE_PIN = 16;
constexpr unsigned long DEBOUNCE_MS = 150;
constexpr unsigned long MQTT_RETRY_MS = 3000;

// TODO: Replace with the actual color sequence after the physical puzzle code is finalized.
constexpr const char* CORRECT_SEQUENCE = "";

struct ColorButton {
    char code;
    int pin;
    int lastState;
    unsigned long stableStart;
};

ColorButton buttons[] = {
    {'R', BUTTON_RED_PIN, HIGH, 0},
    {'B', BUTTON_BLUE_PIN, HIGH, 0},
};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool sequenceEnabled = false;
bool sequenceSolved = false;
String enteredSequence;

void publishPostState();

void resetSequence() {
    sequenceEnabled = false;
    sequenceSolved = false;
    enteredSequence = "";

    for (ColorButton& button : buttons) {
        button.lastState = digitalRead(button.pin);
        button.stableStart = millis();
    }

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

    if (topicText == EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE) {
        sequenceEnabled = message != "off";
        sequenceSolved = false;
        enteredSequence = "";
    } else if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
    } else if (topicText == EscapeTopic::RESET_PUZZLE || topicText == EscapeTopic::LEGACY_GAME_RESET) {
        resetSequence();
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
            mqtt.subscribe(EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE);
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
}

void publishPostState() {
    publishEvent(postStateTopic(5).c_str(), postStatePayload(sequenceSolved));
}

void registerButtonPress(char code) {
    if (!sequenceEnabled || sequenceSolved) {
        return;
    }

    if (String(CORRECT_SEQUENCE).length() == 0) {
        publishEvent(EscapeTopic::COLOR_SEQUENCE_ERROR, "color sequence not configured");
        return;
    }

    enteredSequence += code;

    if (!String(CORRECT_SEQUENCE).startsWith(enteredSequence)) {
        publishEvent(EscapeTopic::COLOR_SEQUENCE_ERROR, "incorrect color sequence");
        enteredSequence = "";
        return;
    }

    if (enteredSequence == CORRECT_SEQUENCE) {
        sequenceSolved = true;
        digitalWrite(LED_PIN, HIGH);
        publishEvent(EscapeTopic::COLOR_SEQUENCE_COMPLETE, "color sequence complete");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);

    for (ColorButton& button : buttons) {
        pinMode(button.pin, INPUT_PULLUP);
        button.lastState = digitalRead(button.pin);
        button.stableStart = millis();
    }

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
        resetSequence();
        delay(500);
    }

    unsigned long now = millis();

    for (ColorButton& button : buttons) {
        int state = digitalRead(button.pin);

        if (state != button.lastState) {
            button.lastState = state;
            button.stableStart = now;
        }

        if (state == LOW && now - button.stableStart >= DEBOUNCE_MS) {
            registerButtonPress(button.code);

            while (digitalRead(button.pin) == LOW) {
                mqtt.loop();
                delay(10);
            }
        }
    }

    delay(20);
}
