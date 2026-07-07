#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    assert(file.good());

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    const std::vector<std::string> activePicoConfigs = {
        "pico2-copper-final-piece/platformio.ini",
        "pico3-painting-rotation/platformio.ini",
        "pico4-smart-film-oven/platformio.ini",
        "pico5-color-buttons/platformio.ini",
        "pico7-fire-panel/platformio.ini",
    };

    for (const std::string& path : activePicoConfigs) {
        std::string text = readFile(path);

        assert(text.find("extra_scripts") != std::string::npos);
        assert(text.find("../tools/platformio_pico_wifi.py") != std::string::npos);
        assert(text.find("EscapeRoom") == std::string::npos);
        assert(text.find("BakeAt350") == std::string::npos);
        assert(text.find("10.42.0.1") == std::string::npos);
        assert(text.find("-D WIFI_SSID") == std::string::npos);
        assert(text.find("-D WIFI_PASS") == std::string::npos);
        assert(text.find("-D MQTT_BROKER=") == std::string::npos);
        assert(text.find("-D MQTT_BROKER_FALLBACK=") == std::string::npos);
        assert(text.find("-D MQTT_BROKER_PORT") == std::string::npos);
    }
}
