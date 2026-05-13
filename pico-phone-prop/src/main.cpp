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
#define MQTT_CLIENT_ID "pico_phone_prop"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int PHONE_PIN = 15;
constexpr int RST_PIN = 14;
constexpr unsigned long DEBOUNCE_MS = 750;
constexpr unsigned long MQTT_RETRY_MS = 3000;
constexpr const char* PHONE_TOPIC = "escape/puzzle/phone/solved";
constexpr const char* PHONE_PAYLOAD = "phone puzzle solved";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool solved = false;
int lastState = LOW;
unsigned long stableStart = 0;

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

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(PHONE_PIN, INPUT);
    pinMode(RST_PIN, INPUT_PULLUP);

    Serial.println("Phone Prop Pico Controller");

    connectWiFi();
    connectMQTT();

    lastState = digitalRead(PHONE_PIN);
    stableStart = millis();
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
        lastState = digitalRead(PHONE_PIN);
        stableStart = millis();
        digitalWrite(LED_PIN, LOW);
        Serial.println("Phone puzzle reset.");
        delay(500);
    }

    int state = digitalRead(PHONE_PIN);
    unsigned long now = millis();

    if (state != lastState) {
        lastState = state;
        stableStart = now;
    }

    if (state == HIGH && !solved && now - stableStart >= DEBOUNCE_MS) {
        solved = true;
        Serial.println("Phone puzzle solved!");
        mqtt.publish(PHONE_TOPIC, PHONE_PAYLOAD);
        digitalWrite(LED_PIN, HIGH);
    }

    delay(50);
}
