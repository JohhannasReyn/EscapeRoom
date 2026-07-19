#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

std::string readText(const std::string& path) {
    std::ifstream file(path);
    assert(file.good());

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    std::string pico2 = readText("pico2-copper-final-piece/src/main.cpp");
    std::string pico4 = readText("pico4-smart-film-oven/src/main.cpp");
    std::string pico5 = readText("pico5-color-buttons/src/main.cpp");
    std::string controller = readText("raspberry-pi-controller/src/GameController.cpp");
    std::string protocol = readText("shared/EscapeRoomProtocol.h");

    assert(pico2.find("ENABLE_COPPER_PUZZLE") == std::string::npos);
    assert(pico2.find("enabled") == std::string::npos);

    assert(pico5.find("ENABLE_COLOR_BUTTON_SEQUENCE") == std::string::npos);
    assert(pico5.find("sequenceEnabled") == std::string::npos);
    assert(pico5.find("enabled=") == std::string::npos);

    assert(pico4.find("ENABLE_OVEN_KNOB") == std::string::npos);
    assert(pico4.find("LEGACY_OVEN_ENABLE") == std::string::npos);
    assert(pico4.find("ovenEnabled") == std::string::npos);
    assert(pico4.find("ovenArmed") != std::string::npos);
    assert(pico4.find("enabled=") == std::string::npos);

    assert(controller.find("ENABLE_COPPER_PUZZLE") == std::string::npos);
    assert(controller.find("ENABLE_COLOR_BUTTON_SEQUENCE") == std::string::npos);
    assert(controller.find("ENABLE_OVEN_KNOB") == std::string::npos);
    assert(controller.find("LEGACY_OVEN_ENABLE") == std::string::npos);
    assert(controller.find("ARM_OVEN_POTENTIOMETER") != std::string::npos);
    assert(controller.find("color button sequence enabled") == std::string::npos);
    assert(controller.find("oven knob armed") != std::string::npos);

    assert(protocol.find("ENABLE_COPPER_PUZZLE") == std::string::npos);
    assert(protocol.find("ENABLE_COLOR_BUTTON_SEQUENCE") == std::string::npos);
    assert(protocol.find("ENABLE_OVEN_KNOB") == std::string::npos);
    assert(protocol.find("LEGACY_OVEN_ENABLE") == std::string::npos);
    assert(protocol.find("ARM_OVEN_POTENTIOMETER") != std::string::npos);
}
