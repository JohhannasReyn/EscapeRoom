#include <Arduino.h>

constexpr int LED_PIN = LED_BUILTIN;
constexpr int COPPER_PIN = 15;

bool copperSolved = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    pinMode(COPPER_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN, LOW);

    Serial.println("Pico 2 copper contact input test started.");
}

void loop() {
    int copperState = digitalRead(COPPER_PIN);

    if (copperState == LOW && !copperSolved) {
        copperSolved = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Copper puzzle complete!");
    }

    delay(50);
}
