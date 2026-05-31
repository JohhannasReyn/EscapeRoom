#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#include "ComponentDiagnostics.h"

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
#define MQTT_CLIENT_ID "pico-0-component-tests"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int WS2812_DATA_PIN = 17;
constexpr int WS2812_TEST_LED_COUNT = 21;
constexpr int WS2812_BRIGHTNESS = 64;
constexpr int PIR_PIN = 6;
constexpr int CONTACT_PIN = 15;
constexpr int BUTTON_PIN = 15;
constexpr int POT_PIN = 26;
constexpr int RELAY_LOCK_PIN = 18;
constexpr int SMART_FILM_RELAY_PIN = 15;

constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long TELEMETRY_MS = 1000;
constexpr unsigned long LED_STEP_MS = 200;
constexpr unsigned long LED_END_PAUSE_MS = 1500;
constexpr unsigned long DIGITAL_DEBOUNCE_MS = 100;
constexpr unsigned long OUTPUT_TOGGLE_MS = 1000;

constexpr const char* TOPIC_ONLINE = "escape/debug/pico0/online";
constexpr const char* TOPIC_CURRENT_TEST = "escape/debug/pico0/current_test";
constexpr const char* TOPIC_WIRING = "escape/debug/pico0/wiring";
constexpr const char* TOPIC_EVENT = "escape/debug/pico0/event";
constexpr const char* TOPIC_TELEMETRY = "escape/debug/pico0/telemetry";
constexpr const char* TOPIC_ERROR = "escape/debug/pico0/error";
constexpr const char* TOPIC_SET_TEST = "escape/debug/pico0/set_test";
constexpr const char* TOPIC_START = "escape/debug/pico0/start";
constexpr const char* TOPIC_STOP = "escape/debug/pico0/stop";
constexpr const char* TOPIC_STATUS = "escape/debug/pico0/status";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_NeoPixel ws2812(WS2812_TEST_LED_COUNT, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

ComponentTest currentTest = ComponentTest::Idle;
bool testRunning = false;
unsigned long lastTelemetryAt = 0;
unsigned long lastLedStepAt = 0;
unsigned long ledPauseUntil = 0;
unsigned long lastOutputToggleAt = 0;
unsigned long digitalStableStart = 0;
int ledStep = 0;
int triggerCount = 0;
int lastDigitalState = LOW;
bool outputState = false;

void publishText(const char* topic, const String& payload) {
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);

    if (mqtt.connected()) {
        mqtt.publish(topic, payload.c_str());
    }
}

void publishCurrentTest() {
    String payload = "test=";
    payload += componentTestName(currentTest);
    payload += ",running=";
    payload += testRunning ? "1" : "0";
    publishText(TOPIC_CURRENT_TEST, payload);
}

void publishWiringGuide() {
    String payload = "test=";
    payload += componentTestName(currentTest);
    payload += ",wiring=";
    payload += componentTestWiring(currentTest);
    publishText(TOPIC_WIRING, payload);
}

void clearWs2812() {
    for (int index = 0; index < WS2812_TEST_LED_COUNT; ++index) {
        ws2812.setPixelColor(index, 0);
    }
    ws2812.show();
}

void resetRuntimeState() {
    triggerCount = 0;
    ledStep = 0;
    ledPauseUntil = 0;
    lastLedStepAt = 0;
    lastTelemetryAt = 0;
    lastOutputToggleAt = 0;
    outputState = false;
    digitalWrite(RELAY_LOCK_PIN, LOW);
    digitalWrite(SMART_FILM_RELAY_PIN, LOW);
    clearWs2812();
}

void configurePinsForTest(ComponentTest test) {
    pinMode(PIR_PIN, INPUT);
    pinMode(CONTACT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(POT_PIN, INPUT);
    pinMode(RELAY_LOCK_PIN, OUTPUT);
    pinMode(SMART_FILM_RELAY_PIN, OUTPUT);

    if (test == ComponentTest::Pir) {
        pinMode(PIR_PIN, INPUT);
        lastDigitalState = digitalRead(PIR_PIN);
    } else if (
        test == ComponentTest::MagneticSwitch ||
        test == ComponentTest::CopperContact ||
        test == ComponentTest::Button
    ) {
        pinMode(CONTACT_PIN, INPUT_PULLUP);
        lastDigitalState = digitalRead(CONTACT_PIN);
    } else {
        lastDigitalState = LOW;
    }

    digitalStableStart = millis();
}

void startTest(ComponentTest test) {
    if (test == ComponentTest::Unknown) {
        publishText(TOPIC_ERROR, "unknown test name");
        return;
    }

    currentTest = test;
    testRunning = currentTest != ComponentTest::Idle;
    resetRuntimeState();
    configurePinsForTest(currentTest);

    String event = "started ";
    event += componentTestName(currentTest);
    publishText(TOPIC_EVENT, event);
    publishCurrentTest();
    publishWiringGuide();
}

void stopTest() {
    testRunning = false;
    resetRuntimeState();
    publishText(TOPIC_EVENT, "stopped");
    publishCurrentTest();
    publishWiringGuide();
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int index = 0; index < length; ++index) {
        message += static_cast<char>(payload[index]);
    }
    message.trim();

    String topicText(topic);
    Serial.print("Command received: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(message);

    if (topicText == TOPIC_SET_TEST || topicText == TOPIC_START) {
        startTest(componentTestFromName(message.c_str()));
    } else if (topicText == TOPIC_STOP) {
        stopTest();
    } else if (topicText == TOPIC_STATUS) {
        publishCurrentTest();
        publishWiringGuide();
    }
}

void blink(int count, int delayMs = 150) {
    for (int index = 0; index < count; ++index) {
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
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
    blink(2);
}

void connectMQTT() {
    mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            mqtt.subscribe(TOPIC_SET_TEST);
            mqtt.subscribe(TOPIC_START);
            mqtt.subscribe(TOPIC_STOP);
            mqtt.subscribe(TOPIC_STATUS);
            blink(3);
            publishText(TOPIC_ONLINE, "online");
            publishCurrentTest();
            publishWiringGuide();
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        delay(MQTT_RETRY_MS);
    }
}

String digitalPayload(const char* pinName, int pin, int state) {
    String payload = "test=";
    payload += componentTestName(currentTest);
    payload += ",pin_name=";
    payload += pinName;
    payload += ",gpio=";
    payload += pin;
    payload += ",state=";
    payload += state;
    payload += ",trigger_count=";
    payload += triggerCount;
    return payload;
}

void checkDigitalInput(const char* pinName, int pin, bool activeLow) {
    int state = digitalRead(pin);
    unsigned long now = millis();

    if (state != lastDigitalState) {
        lastDigitalState = state;
        digitalStableStart = now;
    }

    bool active = activeLow ? state == LOW : state == HIGH;
    if (active && now - digitalStableStart >= DIGITAL_DEBOUNCE_MS) {
        ++triggerCount;
        publishText(TOPIC_EVENT, digitalPayload(pinName, pin, state));

        while (digitalRead(pin) == state) {
            mqtt.loop();
            delay(10);
        }

        lastDigitalState = digitalRead(pin);
        digitalStableStart = millis();
    }
}

void runWs2812Test() {
    unsigned long now = millis();
    if (ledPauseUntil != 0) {
        if (now >= ledPauseUntil) {
            ledPauseUntil = 0;
            ledStep = 0;
            clearWs2812();
        }
        return;
    }

    if (now - lastLedStepAt < LED_STEP_MS) {
        return;
    }

    lastLedStepAt = now;
    int pixel = wrapLedIndex(ledStep, WS2812_TEST_LED_COUNT);
    RgbColor color = rainbowColorForStep(ledStep);
    ws2812.setPixelColor(pixel, ws2812.Color(color.r, color.g, color.b));
    ws2812.show();
    ++ledStep;

    if (ledStep >= WS2812_TEST_LED_COUNT) {
        ++triggerCount;
        publishText(TOPIC_EVENT, "ws2812 cycle complete");
        ledPauseUntil = now + LED_END_PAUSE_MS;
    }
}

void runPotentiometerTest() {
    int raw = analogRead(POT_PIN);
    int percent = map(raw, 0, 4095, 0, 100);
    if (millis() - lastTelemetryAt >= TELEMETRY_MS) {
        lastTelemetryAt = millis();
        String payload = "test=potentiometer,gpio=26,raw=";
        payload += raw;
        payload += ",percent=";
        payload += percent;
        payload += ",trigger_count=";
        payload += triggerCount;
        publishText(TOPIC_TELEMETRY, payload);
    }
}

void runOutputToggleTest(int pin, const char* name) {
    unsigned long now = millis();
    if (now - lastOutputToggleAt >= OUTPUT_TOGGLE_MS) {
        lastOutputToggleAt = now;
        outputState = !outputState;
        digitalWrite(pin, outputState ? HIGH : LOW);
        ++triggerCount;

        String payload = "test=";
        payload += name;
        payload += ",gpio=";
        payload += pin;
        payload += ",state=";
        payload += outputState ? "1" : "0";
        payload += ",trigger_count=";
        payload += triggerCount;
        publishText(TOPIC_EVENT, payload);
    }
}

void publishPeriodicTelemetry() {
    if (!testRunning || currentTest == ComponentTest::Potentiometer || millis() - lastTelemetryAt < TELEMETRY_MS) {
        return;
    }

    lastTelemetryAt = millis();
    String payload = "test=";
    payload += componentTestName(currentTest);
    payload += ",running=1,trigger_count=";
    payload += triggerCount;

    if (currentTest == ComponentTest::Ws2812) {
        payload += ",led_step=";
        payload += ledStep;
        payload += ",led_count=21,gpio=17";
    } else if (currentTest == ComponentTest::Pir) {
        payload += ",gpio=6,state=";
        payload += digitalRead(PIR_PIN);
    } else if (
        currentTest == ComponentTest::MagneticSwitch ||
        currentTest == ComponentTest::CopperContact ||
        currentTest == ComponentTest::Button
    ) {
        payload += ",gpio=15,state=";
        payload += digitalRead(CONTACT_PIN);
    }

    publishText(TOPIC_TELEMETRY, payload);
}

void runCurrentTest() {
    if (!testRunning) {
        return;
    }

    switch (currentTest) {
        case ComponentTest::Ws2812:
            runWs2812Test();
            break;
        case ComponentTest::Pir:
            checkDigitalInput("pir_out", PIR_PIN, false);
            break;
        case ComponentTest::MagneticSwitch:
            checkDigitalInput("magnetic_switch", CONTACT_PIN, true);
            break;
        case ComponentTest::CopperContact:
            checkDigitalInput("copper_contact", CONTACT_PIN, true);
            break;
        case ComponentTest::Button:
            checkDigitalInput("button", BUTTON_PIN, true);
            break;
        case ComponentTest::Potentiometer:
            runPotentiometerTest();
            break;
        case ComponentTest::RelayLock:
            runOutputToggleTest(RELAY_LOCK_PIN, "relay_lock");
            break;
        case ComponentTest::SmartFilmRelay:
            runOutputToggleTest(SMART_FILM_RELAY_PIN, "smart_film_relay");
            break;
        case ComponentTest::Idle:
        case ComponentTest::Unknown:
            break;
    }

    publishPeriodicTelemetry();
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    ws2812.begin();
    ws2812.setBrightness(WS2812_BRIGHTNESS);
    clearWs2812();
    analogReadResolution(12);
    configurePinsForTest(ComponentTest::Idle);

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
    runCurrentTest();
    delay(20);
}
