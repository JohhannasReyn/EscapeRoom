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
    const std::string protocol = readText("shared/EscapeRoomProtocol.h");
    const std::string reportHeader = readText("shared/PicoStatusReport.h");
    const std::string piMain = readText("raspberry-pi-controller/src/main.cpp");

    assert(protocol.find("PICO_STATUS_REPORT") != std::string::npos);
    assert(protocol.find("escape/status/pico") != std::string::npos);
    assert(piMain.find("EscapeTopic::PICO_STATUS_REPORT") != std::string::npos);
    assert(piMain.find("Pico wiring/status report.") != std::string::npos);

    struct ExpectedReport {
        const char* sourcePath;
        const char* title;
        const char* reportConstant;
        const char* wiringText;
    };

    const ExpectedReport reports[] = {
        {
            "pico2-copper-final-piece/src/main.cpp",
            "Pico2 - Copper Puzzle Piece Detection",
            "EscapePicoStatus::PICO2_REPORT",
            "Connected. Wiring: GPIO 15 to puzzle contact 1 and GND to contact 2"
        },
        {
            "pico3-painting-rotation/src/main.cpp",
            "Pico3 - Painting Rotation Sensor",
            "EscapePicoStatus::PICO3_REPORT",
            "Connected. Wiring: GPIO 15 to sensor white/signal, 3V3 to sensor red/VCC, and GND to sensor black/GND. Magnet present pulls GPIO 15 LOW."
        },
        {
            "pico4-smart-film-oven/src/main.cpp",
            "Pico4 - Smart Film, Oven Potentiometer, and Lock",
            "EscapePicoStatus::PICO4_REPORT",
            "Connected. Wiring: GPIO 15 to smart-film relay IN, GPIO 16 to buzzer +, GPIO 18 to lock relay IN, GPIO 26 to oven pot wiper, pot outer legs to 3V3 and GND"
        },
        {
            "pico5-color-buttons/src/main.cpp",
            "Pico5 - Color Button Sequence",
            "EscapePicoStatus::PICO5_REPORT",
            "Connected. Wiring: GPIO 15 red, GPIO 16 green, GPIO 17 yellow, GPIO 18 blue; each button connects its GPIO to GND"
        },
        {
            "pico7-fire-panel/src/main.cpp",
            "Pico7 - Fire Panel Remote",
            "EscapePicoStatus::PICO7_REPORT",
            "Connected. Wiring: GP2-GP11 buttons to GND; GP12-GP21 status LEDs with resistors"
        },
    };

    for (const ExpectedReport& report : reports) {
        const std::string source = readText(report.sourcePath);
        assert(source.find("publishStartupReport") != std::string::npos);
        assert(source.find("EscapeTopic::PICO_STATUS_REPORT") != std::string::npos);
        assert(source.find(report.reportConstant) != std::string::npos);
        assert(reportHeader.find(report.title) != std::string::npos);
        assert(reportHeader.find(report.wiringText) != std::string::npos);
    }
}
