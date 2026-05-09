#include <Arduino.h>

constexpr int LED_PIN = LED_BUILTIN;
constexpr int PUZ_PIN = 15;

bool solved = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    pinMode(PUZ_PIN, INPUT);

    digitalWrite(LED_PIN, LOW);

    Serial.println("Copper puzzle test started.");
}

void loop() {
    int state = digitalRead(PUZ_PIN);

    if (state == HIGH && !solved) {
        solved = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Puzzle solved!");
    }

    delay(50);
}