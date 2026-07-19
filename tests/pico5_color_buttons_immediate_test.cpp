#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream source("pico5-color-buttons/src/main.cpp");
    assert(source.good());

    std::ostringstream buffer;
    buffer << source.rdbuf();
    const std::string text = buffer.str();

    assert(text.find("DEBOUNCE_MS") == std::string::npos);
    assert(text.find("stableStart") == std::string::npos);
    assert(text.find("now - button.stableStart") == std::string::npos);
    assert(text.find("state == LOW && button.lastState == HIGH") != std::string::npos);
    assert(text.find("state == HIGH && button.lastState == LOW") != std::string::npos);
    assert(text.find("registerButtonPress(button)") != std::string::npos);
    assert(text.find("ColorButtonSequence.h") != std::string::npos);
    assert(text.find("sequenceIndex") != std::string::npos);
    assert(text.find("colorButtonCodeMatchesStep(sequenceIndex, pressedButton.code)") != std::string::npos);
    assert(text.find("incorrect color button entry") != std::string::npos);
    assert(text.find("publishAttemptError(\"incorrect color button entry\")") < text.find("++pressedButton.pressCount"));
    assert(text.find("color button counts complete") == std::string::npos);
    assert(text.find("color button sequence complete") != std::string::npos);
    assert(text.find("unsigned long resetCount = 0") != std::string::npos);
    assert(text.find("unsigned long errorCount = 0") != std::string::npos);
    assert(text.find("void publishSensorTelemetry(bool forcePublish = false)") != std::string::npos);
    assert(text.find("++resetCount") != std::string::npos);
    assert(text.find("errorCount = 0") != std::string::npos);
    assert(text.find("++errorCount") != std::string::npos);
    assert(text.find("sequence_index=") != std::string::npos);
    assert(text.find("error_count=") != std::string::npos);
    assert(text.find("reset_count=") != std::string::npos);
    assert(text.find("!forcePublish && millis() - lastSensorTelemetry < SENSOR_TELEMETRY_MS") != std::string::npos);
    assert(text.find("publishSensorTelemetry(true)") < text.find("void publishEvent"));

    std::size_t publishAttemptErrorStart = text.find("void publishAttemptError");
    assert(publishAttemptErrorStart != std::string::npos);
    assert(text.find("++errorCount", publishAttemptErrorStart) < text.find("publishEvent(EscapeTopic::COLOR_SEQUENCE_ERROR", publishAttemptErrorStart));
    std::size_t clearAttemptInError = text.find("clearAttempt();", publishAttemptErrorStart);
    std::size_t telemetryInError = text.find("publishSensorTelemetry(true)", publishAttemptErrorStart);
    assert(clearAttemptInError != std::string::npos);
    assert(telemetryInError != std::string::npos);
    assert(clearAttemptInError < telemetryInError);

    assert(text.rfind("publishSensorTelemetry(true)") > text.find("publishEvent(EscapeTopic::COLOR_SEQUENCE_COMPLETE"));

    return 0;
}
