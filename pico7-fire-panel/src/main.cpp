#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <cstring>

#include "../../shared/EscapeRoomProtocol.h"

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
#define MQTT_CLIENT_ID "pico7-fire-panel"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr unsigned long DEBOUNCE_MS = 120;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long TELEMETRY_MS = 1000;
constexpr unsigned long BLINK_MS = 300;
constexpr unsigned long RESET_HOLD_MS = 5000;
constexpr unsigned long RESET_STAGE_MS = 1000;
constexpr unsigned long RESET_FLASH_ALL_MS = 900;
constexpr int MQTT_ATTEMPTS_PER_HOST = 3;

enum class LedMode {
    Off,
    SolidGreen,
    SolidRed,
    FlashGreen,
    FlashRed,
};

struct FireButton {
    const char* name;
    const char* topic;
    int pin;
    int lastState;
    unsigned long stableStart;
    bool pressRegistered;
};

struct StatusZone {
    const char* name;
    int greenPin;
    int redPin;
    LedMode mode;
};

FireButton fireButtons[] = {
    {"status", EscapeTopic::FIRE_STATUS, 2, HIGH, 0, false},
    {"film-on", EscapeTopic::FIRE_FILM_ON, 3, HIGH, 0, false},
    {"film-off", EscapeTopic::FIRE_FILM_OFF, 4, HIGH, 0, false},
    {"sound-look", EscapeTopic::FIRE_SOUND_LOOK, 5, HIGH, 0, false},
    {"sound-crash", EscapeTopic::FIRE_SOUND_CRASH, 6, HIGH, 0, false},
    {"sound-fail", EscapeTopic::FIRE_SOUND_FAIL, 7, HIGH, 0, false},
    {"sound-pass", EscapeTopic::FIRE_SOUND_PASS, 8, HIGH, 0, false},
    {"sound-bake", EscapeTopic::FIRE_SOUND_BAKE, 9, HIGH, 0, false},
    {"unlock", EscapeTopic::FIRE_UNLOCK, 10, HIGH, 0, false},
    {"reset-all", EscapeTopic::FIRE_RESET_ALL, 11, HIGH, 0, false},
};

StatusZone statusZones[] = {
    {"film", 12, 13, LedMode::FlashRed},
    {"sound", 14, 15, LedMode::FlashRed},
    {"picture", 16, 17, LedMode::FlashRed},
    {"buttons", 18, 19, LedMode::FlashRed},
    {"pot", 20, 21, LedMode::FlashRed},
};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastTelemetryAt = 0;
unsigned long lastBlinkAt = 0;
bool blinkOn = false;
bool resetHoldActive = false;
bool resetPublishedForHold = false;
unsigned long resetHoldStart = 0;
unsigned long resetFlashAllUntil = 0;

void publishEvent(const char* topic, const char* payload);

bool topicMatches(const char* left, const char* right) {
    return strcmp(left, right) == 0;
}

LedMode ledModeFromText(const String& text) {
    if (text == "ready" || text == "solid-green") {
        return LedMode::SolidGreen;
    }

    if (text == "offline" || text == "solid-red") {
        return LedMode::SolidRed;
    }

    if (text == "active" || text == "playing" || text == "triggered" || text == "physical-reset" || text == "flash-green") {
        return LedMode::FlashGreen;
    }

    if (text == "wrong" || text == "error" || text == "checking" || text == "flash-red") {
        return LedMode::FlashRed;
    }

    return LedMode::Off;
}

void setZonePins(StatusZone& zone, bool greenOn, bool redOn) {
    digitalWrite(zone.greenPin, greenOn ? HIGH : LOW);
    digitalWrite(zone.redPin, redOn ? HIGH : LOW);
}

void applyLedMode(StatusZone& zone) {
    switch (zone.mode) {
        case LedMode::SolidGreen:
            setZonePins(zone, true, false);
            break;
        case LedMode::SolidRed:
            setZonePins(zone, false, true);
            break;
        case LedMode::FlashGreen:
            setZonePins(zone, blinkOn, false);
            break;
        case LedMode::FlashRed:
            setZonePins(zone, false, blinkOn);
            break;
        case LedMode::Off:
            setZonePins(zone, false, false);
            break;
    }
}

void updateAllLeds() {
    for (StatusZone& zone : statusZones) {
        applyLedMode(zone);
    }
}

void setAllZones(LedMode mode) {
    for (StatusZone& zone : statusZones) {
        zone.mode = mode;
    }
    updateAllLeds();
}

void setAllRedLeds(bool on) {
    for (StatusZone& zone : statusZones) {
        digitalWrite(zone.redPin, on ? HIGH : LOW);
        digitalWrite(zone.greenPin, LOW);
    }
}

void showResetCountdown(unsigned long heldMs) {
    int activeIndex = static_cast<int>(heldMs / RESET_STAGE_MS);
    if (activeIndex >= 5) {
        activeIndex = 4;
    }

    bool stageBlinkOn = ((heldMs / 250) % 2) == 0;

    for (int index = 0; index < 5; ++index) {
        digitalWrite(statusZones[index].greenPin, LOW);
        digitalWrite(statusZones[index].redPin, index == activeIndex && stageBlinkOn ? HIGH : LOW);
    }
}

bool resetFlashAllActive() {
    if (resetFlashAllUntil == 0) {
        return false;
    }

    if (millis() >= resetFlashAllUntil) {
        resetFlashAllUntil = 0;
        setAllZones(LedMode::FlashRed);
        publishEvent(EscapeTopic::FIRE_STATUS, "status");
        return false;
    }

    bool flashOn = ((millis() / 150) % 2) == 0;
    setAllRedLeds(flashOn);
    return true;
}

void publishEvent(const char* topic, const char* payload) {
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);

    if (mqtt.connected()) {
        mqtt.publish(topic, payload);
    }
}

void handleLedCommand(const String& message) {
    int separator = message.indexOf('=');
    if (separator <= 0 || separator >= static_cast<int>(message.length()) - 1) {
        return;
    }

    String zoneName = message.substring(0, separator);
    String modeText = message.substring(separator + 1);
    zoneName.trim();
    modeText.trim();

    if (zoneName == "all") {
        setAllZones(ledModeFromText(modeText));
        return;
    }

    for (StatusZone& zone : statusZones) {
        if (zoneName == zone.name) {
            zone.mode = ledModeFromText(modeText);
            applyLedMode(zone);
            return;
        }
    }
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    String topicText(topic);

    if (topicText == EscapeTopic::FIRE_PANEL_LED_COMMAND) {
        handleLedCommand(message);
    } else if (topicText == EscapeTopic::STATUS_REQUEST) {
        publishEvent("escape/telemetry/fire-panel/status", "ready");
    }
}

void blinkBuiltIn(int count, int delayMs = 150) {
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
    blinkBuiltIn(2);
}

bool tryConnectMQTT(const char* broker) {
    if (broker == nullptr || broker[0] == '\0') {
        return false;
    }

    mqtt.setServer(broker, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    for (int attempt = 0; attempt < MQTT_ATTEMPTS_PER_HOST && !mqtt.connected(); ++attempt) {
        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(EscapeTopic::FIRE_PANEL_LED_COMMAND);
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            blinkBuiltIn(3);
            publishEvent("escape/telemetry/fire-panel/status", "ready");
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

void publishTelemetry() {
    if (!mqtt.connected() || millis() - lastTelemetryAt < TELEMETRY_MS) {
        return;
    }

    lastTelemetryAt = millis();

    String payload = "ready=1";
    for (StatusZone& zone : statusZones) {
        payload += ",";
        payload += zone.name;
        payload += "=";
        payload += static_cast<int>(zone.mode);
    }

    mqtt.publish("escape/telemetry/fire-panel/status", payload.c_str());
}

void updateBlink() {
    if (resetFlashAllActive() || resetHoldActive) {
        return;
    }

    unsigned long now = millis();
    if (now - lastBlinkAt < BLINK_MS) {
        return;
    }

    lastBlinkAt = now;
    blinkOn = !blinkOn;
    updateAllLeds();
}

void handleResetButton(FireButton& button, int state, unsigned long now) {
    if (state == LOW && !resetHoldActive) {
        resetHoldActive = true;
        resetPublishedForHold = false;
        resetHoldStart = now;
    }

    if (state == LOW && resetHoldActive) {
        unsigned long heldMs = now - resetHoldStart;

        if (!resetPublishedForHold && heldMs >= RESET_HOLD_MS) {
            resetPublishedForHold = true;
            button.pressRegistered = true;
            publishEvent(button.topic, button.name);
            resetFlashAllUntil = now + RESET_FLASH_ALL_MS;
        }

        if (!resetPublishedForHold) {
            showResetCountdown(heldMs);
        }
        return;
    }

    if (state == HIGH && resetHoldActive) {
        resetHoldActive = false;
        resetPublishedForHold = false;
        resetHoldStart = 0;

        if (resetFlashAllUntil == 0) {
            updateAllLeds();
        }
    }

    if (state == HIGH && button.pressRegistered && now - button.stableStart >= DEBOUNCE_MS) {
        button.pressRegistered = false;
    }
}

void checkButtons() {
    unsigned long now = millis();

    for (FireButton& button : fireButtons) {
        int state = digitalRead(button.pin);

        if (state != button.lastState) {
            button.lastState = state;
            button.stableStart = now;
        }

        if (topicMatches(button.topic, EscapeTopic::FIRE_RESET_ALL)) {
            handleResetButton(button, state, now);
            continue;
        }

        if (state == LOW && !button.pressRegistered && now - button.stableStart >= DEBOUNCE_MS) {
            button.pressRegistered = true;
            publishEvent(button.topic, button.name);

            if (topicMatches(button.topic, EscapeTopic::FIRE_STATUS)) {
                setAllZones(LedMode::FlashRed);
            } else if (topicMatches(button.topic, EscapeTopic::FIRE_FILM_ON)) {
                statusZones[0].mode = LedMode::FlashGreen;
                applyLedMode(statusZones[0]);
            } else if (topicMatches(button.topic, EscapeTopic::FIRE_FILM_OFF)) {
                statusZones[0].mode = LedMode::SolidGreen;
                applyLedMode(statusZones[0]);
            } else if (topicMatches(button.topic, EscapeTopic::FIRE_SOUND_FAIL)) {
                statusZones[1].mode = LedMode::FlashRed;
                applyLedMode(statusZones[1]);
            } else if (
                topicMatches(button.topic, EscapeTopic::FIRE_SOUND_LOOK) ||
                topicMatches(button.topic, EscapeTopic::FIRE_SOUND_CRASH) ||
                topicMatches(button.topic, EscapeTopic::FIRE_SOUND_PASS) ||
                topicMatches(button.topic, EscapeTopic::FIRE_SOUND_BAKE)
            ) {
                statusZones[1].mode = LedMode::FlashGreen;
                applyLedMode(statusZones[1]);
            } else if (topicMatches(button.topic, EscapeTopic::FIRE_UNLOCK)) {
                statusZones[4].mode = LedMode::FlashGreen;
                applyLedMode(statusZones[4]);
            }
        }

        if (state == HIGH && button.pressRegistered && now - button.stableStart >= DEBOUNCE_MS) {
            button.pressRegistered = false;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);

    for (FireButton& button : fireButtons) {
        pinMode(button.pin, INPUT_PULLUP);
        button.lastState = digitalRead(button.pin);
        button.stableStart = millis();
    }

    for (StatusZone& zone : statusZones) {
        pinMode(zone.greenPin, OUTPUT);
        pinMode(zone.redPin, OUTPUT);
    }
    setAllZones(LedMode::FlashRed);

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
    checkButtons();
    updateBlink();
    publishTelemetry();
    delay(20);
}
