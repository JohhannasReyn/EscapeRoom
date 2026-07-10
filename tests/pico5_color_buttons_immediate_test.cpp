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

    return 0;
}
