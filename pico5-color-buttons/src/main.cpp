#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../../shared/EscapeRoomProtocol.h"
#include "../../shared/PicoStatusReport.h"
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
#define MQTT_CLIENT_ID "pico5-color-buttons"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int BUTTON_RED_PIN = 15;
constexpr int BUTTON_GREEN_PIN = 16;
constexpr int BUTTON_YELLOW_PIN = 17;
constexpr int BUTTON_BLUE_PIN = 18;
constexpr unsigned long DEBOUNCE_MS = 150;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long SENSOR_TELEMETRY_MS = 1000;
constexpr unsigned long ATTEMPT_TIMEOUT_MS = 10000;
constexpr int MQTT_ATTEMPTS_PER_HOST = 3;
constexpr int REQUIRED_RED_PRESSES = 3;
constexpr int REQUIRED_GREEN_PRESSES = 4;
constexpr int REQUIRED_YELLOW_PRESSES = 2;
constexpr int REQUIRED_BLUE_PRESSES = 3;
constexpr int REQUIRED_TOTAL_PRESSES =
    REQUIRED_RED_PRESSES +
    REQUIRED_GREEN_PRESSES +
    REQUIRED_YELLOW_PRESSES +
    REQUIRED_BLUE_PRESSES;

struct ColorButton {
    char code;
    const char* name;
    int pin;
    int requiredPresses;
    int pressCount;
    int lastState;
    unsigned long stableStart;
    bool pressRegistered;
};

ColorButton buttons[] = {
    {'R', "red", BUTTON_RED_PIN, REQUIRED_RED_PRESSES, 0, HIGH, 0, false},
    {'G', "green", BUTTON_GREEN_PIN, REQUIRED_GREEN_PRESSES, 0, HIGH, 0, false},
    {'Y', "yellow", BUTTON_YELLOW_PIN, REQUIRED_YELLOW_PRESSES, 0, HIGH, 0, false},
    {'B', "blue", BUTTON_BLUE_PIN, REQUIRED_BLUE_PRESSES, 0, HIGH, 0, false},
};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool sequenceSolved = false;
int totalPresses = 0;
unsigned long lastPressAt = 0;
unsigned long lastSensorTelemetry = 0;

void publishPostState();
void publishStartupReport();

void clearAttempt() {
    totalPresses = 0;

    for (ColorButton& button : buttons) {
        button.pressCount = 0;
    }
}

void resetSequence() {
    sequenceSolved = false;
    clearAttempt();
    lastPressAt = 0;

    for (ColorButton& button : buttons) {
        button.lastState = digitalRead(button.pin);
        button.stableStart = millis();
        button.pressRegistered = false;
    }

    digitalWrite(LED_PIN, LOW);
    if (mqtt.connected()) {
        publishPostState();
    }
}

void publishEvent(const char* topic, const char* payload) {
    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }
}

void publishAttemptError(const char* reason) {
    publishEvent(EscapeTopic::COLOR_SEQUENCE_ERROR, reason);
    clearAttempt();
    lastPressAt = millis();
}

bool currentAttemptIsCorrect() {
    if (totalPresses != REQUIRED_TOTAL_PRESSES) {
        return false;
    }

    for (ColorButton& button : buttons) {
        if (button.pressCount != button.requiredPresses) {
            return false;
        }
    }

    return true;
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    String topicText(topic);

    if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
        publishStartupReport();
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

bool tryConnectMQTT(const char* broker) {
    if (broker == nullptr || broker[0] == '\0') {
        return false;
    }

    mqtt.setServer(broker, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    for (int attempt = 0; attempt < MQTT_ATTEMPTS_PER_HOST && !mqtt.connected(); ++attempt) {
        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            mqtt.subscribe(EscapeTopic::RESET_PUZZLE);
            mqtt.subscribe(EscapeTopic::LEGACY_POST_QUERY);
            mqtt.subscribe(EscapeTopic::LEGACY_GAME_RESET);
            blink(3);
            publishPostState();
            publishStartupReport();
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

void publishPostState() {
    publishEvent(postStateTopic(5).c_str(), postStatePayload(sequenceSolved));
}

void publishStartupReport() {
    publishEvent(EscapeTopic::PICO_STATUS_REPORT, EscapePicoStatus::PICO5_REPORT);
}

void publishSensorTelemetry() {
    if (!mqtt.connected() || millis() - lastSensorTelemetry < SENSOR_TELEMETRY_MS) {
        return;
    }

    lastSensorTelemetry = millis();
    String payload = "solved=" + String(sequenceSolved ? 1 : 0);
    payload += ",total_presses=" + String(totalPresses);
    payload += ",required_total=" + String(REQUIRED_TOTAL_PRESSES);

    for (ColorButton& button : buttons) {
        payload += ",";
        payload += button.name;
        payload += "=";
        payload += String(button.pressCount);
        payload += "/";
        payload += String(button.requiredPresses);
        payload += ",pin_";
        payload += button.name;
        payload += "=";
        payload += String(digitalRead(button.pin));
    }

    mqtt.publish("escape/telemetry/pico5/buttons", payload.c_str());
}

void registerButtonPress(ColorButton& pressedButton) {
    if (sequenceSolved) {
        return;
    }

    ++pressedButton.pressCount;
    ++totalPresses;
    lastPressAt = millis();

    if (totalPresses < REQUIRED_TOTAL_PRESSES) {
        return;
    }

    if (currentAttemptIsCorrect()) {
        sequenceSolved = true;
        digitalWrite(LED_PIN, HIGH);
        publishEvent(EscapeTopic::COLOR_SEQUENCE_COMPLETE, "color button counts complete");
        publishPostState();
    } else {
        publishAttemptError("incorrect color button counts");
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

    if (
        !sequenceSolved &&
        totalPresses > 0 &&
        now - lastPressAt >= ATTEMPT_TIMEOUT_MS
    ) {
        publishAttemptError("color button attempt timed out");
    }

    for (ColorButton& button : buttons) {
        int state = digitalRead(button.pin);

        if (state != button.lastState) {
            button.lastState = state;
            button.stableStart = now;
        }

        if (state == LOW && !button.pressRegistered && now - button.stableStart >= DEBOUNCE_MS) {
            button.pressRegistered = true;
            registerButtonPress(button);
        }

        if (state == HIGH && button.pressRegistered && now - button.stableStart >= DEBOUNCE_MS) {
            button.pressRegistered = false;
        }
    }

    publishSensorTelemetry();

    delay(20);
}
