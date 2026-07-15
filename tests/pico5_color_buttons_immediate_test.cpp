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

    return 0;
}
