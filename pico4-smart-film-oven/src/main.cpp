#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../../shared/OvenDial.h"
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
#define MQTT_CLIENT_ID "pico4-smart-film-oven"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int SMART_FILM_PIN = 15;
constexpr int SMART_FILM_BUZZER_PIN = 16;
constexpr int LOCK_PIN = 18;
constexpr int OVEN_POT_PIN = 26;

constexpr int OVEN_MIN_VALUE = 170;
constexpr int OVEN_TARGET_VALUE = 350;
constexpr int OVEN_MAX_VALUE = 440;
constexpr int OVEN_TARGET_TOLERANCE = 10;
constexpr int OVEN_STEP_VALUE = 15;
constexpr int OVEN_POT_MIN_READING = 0;
constexpr int OVEN_POT_MAX_READING = 4095;
constexpr int OVEN_POSITION_PUBLISH_DELTA = OVEN_STEP_VALUE;

constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long SMART_FILM_BUZZER_MS = 350;
constexpr unsigned long OVEN_TARGET_HOLD_MS = 1200;
constexpr unsigned long SENSOR_TELEMETRY_MS = 1000;
constexpr int MQTT_ATTEMPTS_PER_HOST = 3;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool ovenSolved = false;
bool ovenNeedsPhysicalReset = false;
int ovenLastPublishedValue = -1;
unsigned long smartFilmBuzzerOffAt = 0;
unsigned long ovenTargetStableStart = 0;
unsigned long lastSensorTelemetry = 0;

void resetOvenAndOutputs();
void publishPostState();
void publishStartupReport();

void publishEvent(const char* topic, const char* payload) {
    Serial.print("Publishing event: ");
    Serial.println(topic);

    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }
}

void publishOvenDegrees(int ovenDegrees) {
    String payload = String(ovenDegrees);
    publishEvent(EscapeTopic::OVEN_POSITION_UPDATE, payload.c_str());
    mqtt.publish(EscapeTopic::LEGACY_OVEN_DEGREES, payload.c_str());
}

void setLock(bool on) {
    // HIGH = electromagnetic lock released (room unlocked). The release is held
    // until the room is reset so the door stays open; resetOvenAndOutputs()
    // drives LOCK_PIN LOW again to re-arm the lock for the next group.
    digitalWrite(LOCK_PIN, on ? HIGH : LOW);
    digitalWrite(LED_PIN, on ? HIGH : LOW);

    if (on) {
        publishEvent(EscapeTopic::ELECTROMAG_LOCK_UNLOCKED, "electromagnetic lock unlocked");
    }
}

void pulseSmartFilmBuzzer() {
    digitalWrite(SMART_FILM_BUZZER_PIN, HIGH);
    smartFilmBuzzerOffAt = millis() + SMART_FILM_BUZZER_MS;
}

int readOvenPotValue() {
    int rawReading = analogRead(OVEN_POT_PIN);
    return ovenValueFromPotReading(
        rawReading,
        OVEN_POT_MIN_READING,
        OVEN_POT_MAX_READING,
        OVEN_MIN_VALUE,
        OVEN_MAX_VALUE,
        OVEN_STEP_VALUE
    );
}

void publishAndDisplayOvenValue(int ovenValue, bool forcePublish = false) {
    if (
        forcePublish ||
        ovenLastPublishedValue < 0 ||
        abs(ovenValue - ovenLastPublishedValue) >= OVEN_POSITION_PUBLISH_DELTA
    ) {
        publishOvenDegrees(ovenValue);
        ovenLastPublishedValue = ovenValue;
    }
}

void checkOvenPotentiometer() {
    int ovenValue = readOvenPotValue();
    publishAndDisplayOvenValue(ovenValue);
    bool atTarget = ovenValueIsAtTarget(ovenValue, OVEN_TARGET_VALUE, OVEN_TARGET_TOLERANCE);

    if (ovenNeedsPhysicalReset) {
        if (!atTarget) {
            ovenNeedsPhysicalReset = false;
            ovenTargetStableStart = 0;
            publishPostState();
        }
        return;
    }

    if (ovenSolved && !atTarget) {
        ovenSolved = false;
        ovenTargetStableStart = 0;
        publishPostState();
    }

    if (!ovenSolved && atTarget && ovenTargetStableStart == 0) {
        ovenTargetStableStart = millis();
    }

    if (!ovenSolved && !atTarget) {
        ovenTargetStableStart = 0;
    }

    if (!ovenSolved && atTarget && millis() - ovenTargetStableStart >= OVEN_TARGET_HOLD_MS) {
        ovenSolved = true;
        publishAndDisplayOvenValue(ovenValue, true);
        publishEvent(EscapeTopic::OVEN_TARGET_REACHED, "oven target reached");
        setLock(true);
    }
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    String topicText(topic);

    if (topicText == EscapeTopic::REVEAL_SMART_FILM || topicText == EscapeTopic::LEGACY_PDLC_ON) {
        bool revealSmartFilm = message != "off";
        digitalWrite(SMART_FILM_PIN, revealSmartFilm ? HIGH : LOW);

        if (revealSmartFilm) {
            pulseSmartFilmBuzzer();
        }

        publishEvent(EscapeTopic::SMART_FILM_READY, revealSmartFilm ? "transparent" : "opaque");
    } else if (topicText == EscapeTopic::UNLOCK_ELECTROMAG_LOCK || topicText == EscapeTopic::LEGACY_LOCK_TRIGGER) {
        setLock(message != "off");
    } else if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
        publishStartupReport();
    } else if (topicText == EscapeTopic::RESET_PUZZLE || topicText == EscapeTopic::LEGACY_GAME_RESET) {
        resetOvenAndOutputs();
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
            mqtt.subscribe(EscapeTopic::REVEAL_SMART_FILM);
            mqtt.subscribe(EscapeTopic::UNLOCK_ELECTROMAG_LOCK);
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            mqtt.subscribe(EscapeTopic::RESET_PUZZLE);
            mqtt.subscribe(EscapeTopic::LEGACY_PDLC_ON);
            mqtt.subscribe(EscapeTopic::LEGACY_LOCK_TRIGGER);
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
    publishEvent(postStateTopic(4).c_str(), postStatePayload(ovenSolved));
}

void publishStartupReport() {
    publishEvent(EscapeTopic::PICO_STATUS_REPORT, EscapePicoStatus::PICO4_REPORT);
}

void publishSensorTelemetry() {
    if (!mqtt.connected() || millis() - lastSensorTelemetry < SENSOR_TELEMETRY_MS) {
        return;
    }

    lastSensorTelemetry = millis();
    int rawPot = analogRead(OVEN_POT_PIN);
    int ovenValue = ovenValueFromPotReading(
        rawPot,
        OVEN_POT_MIN_READING,
        OVEN_POT_MAX_READING,
        OVEN_MIN_VALUE,
        OVEN_MAX_VALUE,
        OVEN_STEP_VALUE
    );
    String payload = "oven_raw=" + String(rawPot);
    payload += ",oven_value=" + String(ovenValue);
    payload += ",solved=" + String(ovenSolved ? 1 : 0);
    payload += ",needs_reset=" + String(ovenNeedsPhysicalReset ? 1 : 0);
    payload += ",smart_film=" + String(digitalRead(SMART_FILM_PIN));
    payload += ",smart_film_buzzer=" + String(digitalRead(SMART_FILM_BUZZER_PIN));
    payload += ",lock=" + String(digitalRead(LOCK_PIN));
    mqtt.publish("escape/telemetry/pico4/oven", payload.c_str());
}

void resetOvenAndOutputs() {
    ovenSolved = false;
    int ovenValue = readOvenPotValue();
    ovenNeedsPhysicalReset = ovenValueIsAtTarget(ovenValue, OVEN_TARGET_VALUE, OVEN_TARGET_TOLERANCE);
    ovenLastPublishedValue = -1;
    ovenTargetStableStart = 0;
    setLock(false);
    digitalWrite(SMART_FILM_PIN, LOW);
    digitalWrite(SMART_FILM_BUZZER_PIN, LOW);
    smartFilmBuzzerOffAt = 0;
    digitalWrite(LED_PIN, LOW);
    if (mqtt.connected()) {
        publishAndDisplayOvenValue(ovenValue, true);
        publishPostState();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);
    pinMode(SMART_FILM_PIN, OUTPUT);
    pinMode(SMART_FILM_BUZZER_PIN, OUTPUT);
    pinMode(LOCK_PIN, OUTPUT);
    pinMode(OVEN_POT_PIN, INPUT);
    analogReadResolution(12);

    resetOvenAndOutputs();
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
        resetOvenAndOutputs();
        delay(500);
    }

    checkOvenPotentiometer();
    publishSensorTelemetry();

    if (smartFilmBuzzerOffAt != 0 && millis() >= smartFilmBuzzerOffAt) {
        digitalWrite(SMART_FILM_BUZZER_PIN, LOW);
        smartFilmBuzzerOffAt = 0;
    }

    delay(50);
}
