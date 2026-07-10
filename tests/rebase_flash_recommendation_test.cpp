#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::string readText(const std::string& path) {
    std::ifstream file(path);
    assert(file.good());

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    const std::string gitignore = readText(".gitignore");
    const std::string rebase = readText("tools/rebase.sh");
    const std::string help = readText("tools/help.sh");
    const std::string readme = readText("README.md");

    assert(gitignore.find(".escape-room-rebase-state") != std::string::npos);

    assert(rebase.find("REBASE_STATE_FILE") != std::string::npos);
    assert(rebase.find(".escape-room-rebase-state") != std::string::npos);
    assert(rebase.find("hash_pico_inputs") != std::string::npos);
    assert(rebase.find("print_flash_recommendations") != std::string::npos);
    assert(rebase.find("write_rebase_state") != std::string::npos);
    assert(rebase.find("No previous Pico firmware rebase state was found") != std::string::npos);
    assert(rebase.find("No Pico firmware changes detected since the last rebase") != std::string::npos);
    assert(rebase.find("Pico firmware changed since the last rebase") != std::string::npos);
    assert(rebase.find("tools/flash-pico.sh all") != std::string::npos);
    assert(rebase.find("shared") != std::string::npos);
    assert(rebase.find("tools/platformio_pico_wifi.py") != std::string::npos);

    const std::vector<std::pair<std::string, std::string>> picos = {
        {"pico2", "pico2-copper-final-piece"},
        {"pico3", "pico3-painting-rotation"},
        {"pico4", "pico4-smart-film-oven"},
        {"pico5", "pico5-color-buttons"},
        {"pico7", "pico7-fire-panel"},
    };

    for (const auto& pico : picos) {
        assert(rebase.find(pico.first) != std::string::npos);
        assert(rebase.find(pico.second) != std::string::npos);
        assert(rebase.find("tools/flash-pico.sh " + pico.first) != std::string::npos);
    }

    assert(help.find("which Picos need to be flashed") != std::string::npos);
    assert(readme.find("which Picos need to be flashed") != std::string::npos);
}
