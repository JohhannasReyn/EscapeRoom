#include <Arduino.h>

// Pico 6 is intentionally unused in the active five-Pico room layout.
// Historical puzzle firmware is preserved in unused-future-puzzles/.

constexpr int LED_PIN = LED_BUILTIN;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    Serial.begin(115200);
    delay(1500);
    Serial.println("Pico 6 unused / future puzzle archive. Not part of active runtime.");
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(1900);
}
