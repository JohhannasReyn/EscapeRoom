#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream source("pico4-smart-film-oven/src/main.cpp");
    assert(source.good());

    std::ostringstream buffer;
    buffer << source.rdbuf();
    const std::string text = buffer.str();

    assert(text.find("constexpr int OVEN_MIN_VALUE = 170") != std::string::npos);
    assert(text.find("constexpr int OVEN_MAX_VALUE = 440") != std::string::npos);
    assert(text.find("constexpr int OVEN_STEP_VALUE = 15") != std::string::npos);
    assert(text.find("OVEN_POSITION_PUBLISH_DELTA = OVEN_STEP_VALUE") != std::string::npos);
    assert(text.find("OVEN_STEP_VALUE") < text.find("OVEN_POSITION_PUBLISH_DELTA"));
    assert(text.find("OVEN_STEP_VALUE") < text.find("readOvenPotValue"));

    return 0;
}
