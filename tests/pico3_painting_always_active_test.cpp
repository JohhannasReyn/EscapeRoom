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
    assert(text.find("if (state == LOW)") != std::string::npos);
    assert(text.find("paintingTriggered = false;") != std::string::npos);

    std::size_t highTrigger = text.find("if (state == HIGH && !paintingTriggered)");
    assert(highTrigger != std::string::npos);

    std::size_t publish = text.find("publishEvent(EscapeTopic::PAINTING_ROTATION_COMPLETE", highTrigger);
    assert(publish != std::string::npos);

    std::string highPath = text.substr(highTrigger, publish - highTrigger);
    assert(highPath.find("paintingStableStart") == std::string::npos);
    assert(highPath.find("DEBOUNCE_MS") == std::string::npos);
    assert(highPath.find("PAINTING_REARM_MS") == std::string::npos);
    assert(highPath.find("now -") == std::string::npos);
}
