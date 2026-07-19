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
    assert(text.find("constexpr unsigned long ROOM_COMPLETE_RESET_DELAY_MS = 10000") == std::string::npos);
    assert(text.find("bool ovenArmed = false") != std::string::npos);
    assert(text.find("EscapeTopic::ARM_OVEN_POTENTIOMETER") != std::string::npos);
    assert(text.find("mqtt.subscribe(EscapeTopic::ARM_OVEN_POTENTIOMETER)") != std::string::npos);
    assert(text.find("if (!ovenArmed)") != std::string::npos);
    assert(text.find("oven_armed=") != std::string::npos);
    assert(text.find("OVEN_POSITION_PUBLISH_DELTA = OVEN_STEP_VALUE") != std::string::npos);
    assert(text.find("OVEN_STEP_VALUE") < text.find("OVEN_POSITION_PUBLISH_DELTA"));
    assert(text.find("OVEN_STEP_VALUE") < text.find("readOvenPotValue"));

    return 0;
}
