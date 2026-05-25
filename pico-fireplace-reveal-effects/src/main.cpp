#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../../shared/PostState.h"
#include "../../shared/EncoderDial.h"

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
#define MQTT_CLIENT_ID "pico_fireplace_reveal_effects"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int RST_PIN = 14;
constexpr int PDLC_PIN = 15;
constexpr int SMOKE_PIN = 16;
constexpr int FIREPLACE_PIN = 17;
constexpr int LOCK_PIN = 18;
constexpr int OVEN_ENCODER_CLK_PIN = 19;
constexpr int OVEN_ENCODER_DT_PIN = 20;
constexpr int OVEN_ENCODER_SW_PIN = 21;
constexpr int OVEN_TARGET_DEGREES = 350;
constexpr int OVEN_DEGREES_PER_STEP = 10;
constexpr int OVEN_TOLERANCE_DEGREES = 5;

constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr unsigned long SMOKE_BURST_MS = 1200;
constexpr unsigned long OVEN_LOCK_RELEASE_MS = 100;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool fireplaceSolved = false;
int fireplaceLastState = LOW;
unsigned long fireplaceStableStart = 0;
unsigned long smokeOffAt = 0;
unsigned long lockOffAt = 0;
bool ovenEnabled = false;
bool ovenSolved = false;
int ovenEncoderSteps = 0;
int ovenLastClk = HIGH;

void resetFireplaceAndOutputs();
void publishOvenDegrees(int ovenDegrees);
void publishPostState();

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

    if (String(topic) == "escape/post/query") {
        publishPostState();
    } else if (String(topic) == "escape/game/reset") {
        resetFireplaceAndOutputs();
    } else if (String(topic) == "escape/oven/enable") {
        ovenEnabled = message != "off";
        ovenSolved = false;
        ovenEncoderSteps = 0;
        ovenLastClk = digitalRead(OVEN_ENCODER_CLK_PIN);
        if (ovenEnabled) {
            publishOvenDegrees(0);
        }
        Serial.println(ovenEnabled ? "Oven dial enabled." : "Oven dial disabled.");
    } else if (String(topic) == "escape/pdlc/on") {
        digitalWrite(PDLC_PIN, message == "off" ? LOW : HIGH);
    } else if (String(topic) == "escape/smoke/burst") {
        pulseOutput(SMOKE_PIN, SMOKE_BURST_MS, smokeOffAt);
    } else if (String(topic) == "escape/lock/trigger") {
        pulseOutput(LOCK_PIN, OVEN_LOCK_RELEASE_MS, lockOffAt);
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
    mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            Serial.println("MQTT connected.");
            mqtt.subscribe("escape/pdlc/on");
            mqtt.subscribe("escape/smoke/burst");
            mqtt.subscribe("escape/lock/trigger");
            mqtt.subscribe("escape/oven/enable");
            mqtt.subscribe("escape/post/query");
            mqtt.subscribe("escape/game/reset");
            blink(3);
            return;
        }

        Serial.print("MQTT failed, state=");
        Serial.println(mqtt.state());
        delay(MQTT_RETRY_MS);
    }
}

void publishFireplaceSolved() {
    const char* topic = "escape/puzzle/fireplace/solved";
    const char* payload = "fireplace puzzle solved";

    Serial.print("Publishing event: ");
    Serial.println(topic);

    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void publishOvenSolved() {
    const char* topic = "escape/puzzle/oven/solved";
    const char* payload = "oven dial set to 350 degrees";

    Serial.print("Publishing event: ");
    Serial.println(topic);

    if (!mqtt.publish(topic, payload)) {
        Serial.println("MQTT publish failed.");
    }

    digitalWrite(LED_PIN, HIGH);
}

void publishOvenDegrees(int ovenDegrees) {
    const char* topic = "escape/oven/degrees";
    String payload = String(ovenDegrees);

    Serial.print("Publishing oven degrees: ");
    Serial.println(payload);

    if (!mqtt.publish(topic, payload.c_str())) {
        Serial.println("MQTT publish failed.");
    }
}

void publishPostState() {
    std::string topic = postStateTopic(4);
    const char* payload = postStatePayload(digitalRead(FIREPLACE_PIN) == HIGH);

    Serial.print("Publishing POST state: ");
    Serial.print(topic.c_str());
    Serial.print(" -> ");
    Serial.println(payload);

    if (!mqtt.publish(topic.c_str(), payload)) {
        Serial.println("MQTT publish failed.");
    }
}

void resetFireplaceAndOutputs() {
    fireplaceSolved = false;
    fireplaceLastState = digitalRead(FIREPLACE_PIN);
    fireplaceStableStart = millis();
    smokeOffAt = 0;
    lockOffAt = 0;
    ovenEnabled = false;
    ovenSolved = false;
    ovenEncoderSteps = 0;
    ovenLastClk = digitalRead(OVEN_ENCODER_CLK_PIN);

    digitalWrite(PDLC_PIN, LOW);
    digitalWrite(SMOKE_PIN, LOW);
    digitalWrite(LOCK_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    if (mqtt.connected()) {
        publishPostState();
    }

    Serial.println("Fireplace/reveal zone reset.");
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RST_PIN, INPUT_PULLUP);
    pinMode(PDLC_PIN, OUTPUT);
    pinMode(SMOKE_PIN, OUTPUT);
    pinMode(FIREPLACE_PIN, INPUT);
    pinMode(LOCK_PIN, OUTPUT);
    pinMode(OVEN_ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(OVEN_ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(OVEN_ENCODER_SW_PIN, INPUT_PULLUP);

    digitalWrite(PDLC_PIN, LOW);
    digitalWrite(SMOKE_PIN, LOW);
    digitalWrite(LOCK_PIN, LOW);

    Serial.println("Fireplace Reveal Effects Pico");

    fireplaceLastState = digitalRead(FIREPLACE_PIN);
    fireplaceStableStart = millis();
    ovenLastClk = digitalRead(OVEN_ENCODER_CLK_PIN);

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
        resetFireplaceAndOutputs();
        delay(500);
    }

    unsigned long now = millis();

    int fireplaceState = digitalRead(FIREPLACE_PIN);

    if (fireplaceState != fireplaceLastState) {
        fireplaceLastState = fireplaceState;
        fireplaceStableStart = now;
    }

    if (fireplaceState == HIGH && !fireplaceSolved && now - fireplaceStableStart >= DEBOUNCE_MS) {
        fireplaceSolved = true;
        Serial.println("Fireplace puzzle solved!");
        publishFireplaceSolved();
    }

    if (ovenEnabled && !ovenSolved) {
        int clk = digitalRead(OVEN_ENCODER_CLK_PIN);

        if (clk != ovenLastClk && clk == LOW) {
            int dt = digitalRead(OVEN_ENCODER_DT_PIN);
            ovenEncoderSteps += dt == HIGH ? 1 : -1;

            int ovenDegrees = encoderDegreesFromSteps(ovenEncoderSteps, OVEN_DEGREES_PER_STEP);
            Serial.print("Oven dial degrees: ");
            Serial.println(ovenDegrees);
            publishOvenDegrees(ovenDegrees);

            if (dialIsAtTarget(ovenDegrees, OVEN_TARGET_DEGREES, OVEN_TOLERANCE_DEGREES)) {
                ovenSolved = true;
                Serial.println("Oven dial solved!");
                publishOvenSolved();
            }
        }

        ovenLastClk = clk;
    }

    if (smokeOffAt != 0 && now >= smokeOffAt) {
        digitalWrite(SMOKE_PIN, LOW);
        smokeOffAt = 0;
    }

    if (lockOffAt != 0 && now >= lockOffAt) {
        digitalWrite(LOCK_PIN, LOW);
        lockOffAt = 0;
    }

    if (smokeOffAt == 0 && lockOffAt == 0) {
        digitalWrite(LED_PIN, digitalRead(PDLC_PIN));
    }

    delay(50);
}
