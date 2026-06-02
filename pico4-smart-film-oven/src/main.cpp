#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#include "../../shared/OvenDial.h"
#include "../../shared/EscapeRoomProtocol.h"
#include "../../shared/OvenThermometer.h"
#include "../../shared/PostState.h"

#ifndef WIFI_SSID
#define WIFI_SSID "EscapeRoom"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "BakeAt350"
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER "10.42.0.1"
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
constexpr int THERMOMETER_LED_PIN = 17;
constexpr int LOCK_PIN = 18;
constexpr int OVEN_POT_PIN = 26;

constexpr int OVEN_MIN_VALUE = 0;
constexpr int OVEN_TARGET_VALUE = 350;
constexpr int OVEN_MAX_VALUE = 500;
constexpr int OVEN_TARGET_TOLERANCE = 10;
constexpr int OVEN_POT_MIN_READING = 0;
constexpr int OVEN_POT_MAX_READING = 4095;
constexpr int OVEN_POSITION_PUBLISH_DELTA = 2;
constexpr int THERMOMETER_LED_COUNT = 12;
constexpr int THERMOMETER_BRIGHTNESS = 64;

constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long OVEN_LOCK_RELEASE_MS = 100;
constexpr unsigned long SMART_FILM_BUZZER_MS = 350;
constexpr unsigned long SENSOR_TELEMETRY_MS = 1000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_NeoPixel thermometerStrip(THERMOMETER_LED_COUNT, THERMOMETER_LED_PIN, NEO_GRB + NEO_KHZ800);

bool ovenEnabled = false;
bool ovenSolved = false;
int ovenLastPublishedValue = -1;
unsigned long lockOffAt = 0;
unsigned long smartFilmBuzzerOffAt = 0;
unsigned long lastSensorTelemetry = 0;

void resetOvenAndOutputs();
void publishPostState();

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
    digitalWrite(LOCK_PIN, on ? HIGH : LOW);
    digitalWrite(LED_PIN, on ? HIGH : LOW);

    if (on) {
        lockOffAt = millis() + OVEN_LOCK_RELEASE_MS;
        publishEvent(EscapeTopic::ELECTROMAG_LOCK_UNLOCKED, "electromagnetic lock unlocked");
    } else {
        lockOffAt = 0;
    }
}

void pulseSmartFilmBuzzer() {
    digitalWrite(SMART_FILM_BUZZER_PIN, HIGH);
    smartFilmBuzzerOffAt = millis() + SMART_FILM_BUZZER_MS;
}

void updateThermometer(int ovenValue) {
    int litCount = litThermometerLedCount(ovenValue, OVEN_MIN_VALUE, OVEN_MAX_VALUE, THERMOMETER_LED_COUNT);

    for (int index = 0; index < THERMOMETER_LED_COUNT; ++index) {
        RgbColor color = thermometerColor(thermometerBandForIndex(index, litCount));
        thermometerStrip.setPixelColor(index, thermometerStrip.Color(color.r, color.g, color.b));
    }

    thermometerStrip.show();
}

void clearThermometer() {
    updateThermometer(OVEN_MIN_VALUE);
}

int readOvenPotValue() {
    int rawReading = analogRead(OVEN_POT_PIN);
    return ovenValueFromPotReading(
        rawReading,
        OVEN_POT_MIN_READING,
        OVEN_POT_MAX_READING,
        OVEN_MIN_VALUE,
        OVEN_MAX_VALUE
    );
}

void publishAndDisplayOvenValue(int ovenValue, bool forcePublish = false) {
    updateThermometer(ovenValue);

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
    if (!ovenEnabled) {
        return;
    }

    int ovenValue = readOvenPotValue();
    publishAndDisplayOvenValue(ovenValue);
    bool atTarget = ovenValueIsAtTarget(ovenValue, OVEN_TARGET_VALUE, OVEN_TARGET_TOLERANCE);

    if (ovenSolved && !atTarget) {
        ovenSolved = false;
        publishPostState();
    }

    if (!ovenSolved && atTarget) {
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
    } else if (topicText == EscapeTopic::ENABLE_OVEN_KNOB || topicText == EscapeTopic::LEGACY_OVEN_ENABLE) {
        ovenEnabled = message != "off";
        ovenSolved = false;
        ovenLastPublishedValue = -1;
        clearThermometer();
    } else if (topicText == EscapeTopic::UNLOCK_ELECTROMAG_LOCK || topicText == EscapeTopic::LEGACY_LOCK_TRIGGER) {
        setLock(message != "off");
    } else if (topicText == EscapeTopic::STATUS_REQUEST || topicText == EscapeTopic::LEGACY_POST_QUERY) {
        publishPostState();
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

void connectMQTT() {
    mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(EscapeTopic::REVEAL_SMART_FILM);
            mqtt.subscribe(EscapeTopic::ENABLE_OVEN_KNOB);
            mqtt.subscribe(EscapeTopic::UNLOCK_ELECTROMAG_LOCK);
            mqtt.subscribe(EscapeTopic::STATUS_REQUEST);
            mqtt.subscribe(EscapeTopic::RESET_PUZZLE);
            mqtt.subscribe(EscapeTopic::LEGACY_PDLC_ON);
            mqtt.subscribe(EscapeTopic::LEGACY_OVEN_ENABLE);
            mqtt.subscribe(EscapeTopic::LEGACY_LOCK_TRIGGER);
            mqtt.subscribe(EscapeTopic::LEGACY_POST_QUERY);
            mqtt.subscribe(EscapeTopic::LEGACY_GAME_RESET);
            blink(3);
            publishPostState();
            return;
        }

        delay(MQTT_RETRY_MS);
    }
}

void publishPostState() {
    publishEvent(postStateTopic(4).c_str(), postStatePayload(ovenSolved));
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
        OVEN_MAX_VALUE
    );
    String payload = "oven_raw=" + String(rawPot);
    payload += ",oven_value=" + String(ovenValue);
    payload += ",enabled=" + String(ovenEnabled ? 1 : 0);
    payload += ",solved=" + String(ovenSolved ? 1 : 0);
    payload += ",smart_film=" + String(digitalRead(SMART_FILM_PIN));
    payload += ",smart_film_buzzer=" + String(digitalRead(SMART_FILM_BUZZER_PIN));
    payload += ",lock=" + String(digitalRead(LOCK_PIN));
    mqtt.publish("escape/telemetry/pico4/oven", payload.c_str());
}

void resetOvenAndOutputs() {
    ovenEnabled = false;
    ovenSolved = false;
    ovenLastPublishedValue = -1;
    setLock(false);
    digitalWrite(SMART_FILM_PIN, LOW);
    digitalWrite(SMART_FILM_BUZZER_PIN, LOW);
    smartFilmBuzzerOffAt = 0;
    digitalWrite(LED_PIN, LOW);
    clearThermometer();
    if (mqtt.connected()) {
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

    thermometerStrip.begin();
    thermometerStrip.setBrightness(THERMOMETER_BRIGHTNESS);
    clearThermometer();

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

    if (lockOffAt != 0 && millis() >= lockOffAt) {
        setLock(false);
    }

    if (smartFilmBuzzerOffAt != 0 && millis() >= smartFilmBuzzerOffAt) {
        digitalWrite(SMART_FILM_BUZZER_PIN, LOW);
        smartFilmBuzzerOffAt = 0;
    }

    delay(50);
}
