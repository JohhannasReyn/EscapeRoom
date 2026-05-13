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
#define MQTT_CLIENT_ID "pico_cubby_controller"
#endif

constexpr int LED_PIN = LED_BUILTIN;
constexpr int CUBBY_1_LIGHT_PIN = 15;
constexpr int CUBBY_2_UNLOCK_PIN = 16;
constexpr unsigned long MQTT_RETRY_MS = 3000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setOutput(int pin, bool on) {
    digitalWrite(pin, on ? HIGH : LOW);
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    bool on = message != "off";

    Serial.print("Command received: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(message);

    if (String(topic) == "escape/cubby/1/light_on") {
        setOutput(CUBBY_1_LIGHT_PIN, on);
    } else if (String(topic) == "escape/cubby/2/unlock") {
        setOutput(CUBBY_2_UNLOCK_PIN, on);
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
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(handleMessage);

    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(MQTT_BROKER);

        if (mqtt.connect(MQTT_CLIENT_ID)) {
            Serial.println("MQTT connected.");
            mqtt.subscribe("escape/cubby/1/light_on");
            mqtt.subscribe("escape/cubby/2/unlock");
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
    pinMode(CUBBY_1_LIGHT_PIN, OUTPUT);
    pinMode(CUBBY_2_UNLOCK_PIN, OUTPUT);

    digitalWrite(CUBBY_1_LIGHT_PIN, LOW);
    digitalWrite(CUBBY_2_UNLOCK_PIN, LOW);

    Serial.println("Cubby Controller Pico");

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
}
