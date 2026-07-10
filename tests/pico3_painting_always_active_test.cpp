#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream source("pico3-painting-rotation/src/main.cpp");
    assert(source.good());

    std::ostringstream buffer;
    buffer << source.rdbuf();
    std::string text = buffer.str();

    assert(text.find("paintingEnabled") == std::string::npos);
    assert(text.find("ENABLE_PAINTING_ROTATION") == std::string::npos);
    assert(text.find("enabled=") == std::string::npos);
    assert(text.find("PAINTING_ROTATION_COMPLETE") != std::string::npos);
    assert(text.find("paintingTriggerCount") != std::string::npos);
    assert(text.find("constexpr unsigned long DEBOUNCE_MS = 750") == std::string::npos);
    assert(text.find("PAINTING_REARM_MS") == std::string::npos);
    assert(text.find("paintingLowStart") == std::string::npos);
    assert(text.find("paintingNeedsPhysicalReset") != std::string::npos);
    assert(text.find("paintingNeedsPhysicalReset = state == PAINTING_SENSOR_ACTIVE_STATE") != std::string::npos);
    assert(text.find("if (paintingNeedsPhysicalReset)") != std::string::npos);
    assert(text.find("postStatePayload(paintingTriggered)") != std::string::npos);
    assert(text.find("constexpr int PAINTING_SENSOR_ACTIVE_STATE = LOW") != std::string::npos);
    assert(text.find("constexpr int PAINTING_SENSOR_IDLE_STATE = HIGH") != std::string::npos);
    assert(text.find("pinMode(PAINTING_SENSOR_PIN, INPUT_PULLUP)") != std::string::npos);

    std::size_t highRearm = text.find("if (state == PAINTING_SENSOR_IDLE_STATE)");
    assert(highRearm != std::string::npos);

    std::size_t rearm = text.find("paintingTriggered = false;", highRearm);
    assert(rearm != std::string::npos);

    std::size_t lowTrigger = text.find("if (state == PAINTING_SENSOR_ACTIVE_STATE && !paintingTriggered)");
    assert(lowTrigger != std::string::npos);

    std::size_t publish = text.find("publishEvent(EscapeTopic::PAINTING_ROTATION_COMPLETE", lowTrigger);
    assert(publish != std::string::npos);

    std::string lowPath = text.substr(lowTrigger, publish - lowTrigger);
    assert(lowPath.find("paintingStableStart") == std::string::npos);
    assert(lowPath.find("DEBOUNCE_MS") == std::string::npos);
    assert(lowPath.find("PAINTING_REARM_MS") == std::string::npos);
    assert(lowPath.find("now -") == std::string::npos);
}
