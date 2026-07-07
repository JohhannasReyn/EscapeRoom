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
}
