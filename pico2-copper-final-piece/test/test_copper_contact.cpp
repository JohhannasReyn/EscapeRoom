#include <Arduino.h>

constexpr int LED_PIN = LED_BUILTIN;
constexpr int COPPER_PIN = 15;
constexpr int FINAL_PIECE_PIN = 16;

bool copperSolved = false;
bool finalPiecePlaced = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    pinMode(COPPER_PIN, INPUT);
    pinMode(FINAL_PIECE_PIN, INPUT);

    digitalWrite(LED_PIN, LOW);

    Serial.println("Pico 2 copper/final piece input test started.");
}

void loop() {
    int copperState = digitalRead(COPPER_PIN);
    int finalPieceState = digitalRead(FINAL_PIECE_PIN);

    if (copperState == HIGH && !copperSolved) {
        copperSolved = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Copper puzzle complete!");
    }

    if (finalPieceState == HIGH && !finalPiecePlaced) {
        finalPiecePlaced = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Final puzzle piece placed!");
    }

    delay(50);
}
