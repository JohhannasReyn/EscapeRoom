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
    const std::string config = readText("pico-wifi.env");
    const std::string example = readText("pico-wifi.env.example");
    const std::string rebase = readText("tools/rebase.sh");
    const std::string flash = readText("tools/flash-pico.sh");
    const std::string help = readText("tools/help.sh");

    assert(config.find("WIFI_SSID=\"EscapeRoom\"") != std::string::npos);
    assert(config.find("WIFI_PASS=\"BakeAt350\"") != std::string::npos);
    assert(config.find("MQTT_BROKER=\"ceenypie.local\"") != std::string::npos);

    assert(example.find("WIFI_SSID=\"EscapeRoom\"") != std::string::npos);
    assert(example.find("WIFI_PASS=\"BakeAt350\"") != std::string::npos);

    assert(rebase.find("pico2-copper-final-piece") != std::string::npos);
    assert(rebase.find("pico3-painting-rotation") != std::string::npos);
    assert(rebase.find("pico4-smart-film-oven") != std::string::npos);
    assert(rebase.find("pico5-color-buttons") != std::string::npos);
    assert(rebase.find("pico7-fire-panel") != std::string::npos);
    assert(rebase.find("pico-wifi.env") != std::string::npos);

    assert(flash.find("tools/flash-pico.sh all") != std::string::npos);
    assert(flash.find("pico-wifi.env") != std::string::npos);
    assert(help.find("tools/flash-pico.sh all") != std::string::npos);
    assert(help.find("tools/help.sh") != std::string::npos);
}
