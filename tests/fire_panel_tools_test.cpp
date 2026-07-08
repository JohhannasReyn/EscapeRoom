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
    const std::string rebase = readText("tools/rebase.sh");
    const std::string help = readText("tools/help.sh");
    const std::string installHelp = readText("tools/install-help-command.sh");
    const std::string capture = readText("tools/capture-fire-panel-buttons.sh");
    const std::string pico7 = readText("pico7-fire-panel/src/main.cpp");
    const std::string controller = readText("raspberry-pi-controller/src/GameController.cpp");

    assert(rebase.find("chmod +x tools/*.sh") != std::string::npos);
    assert(rebase.find("chmod +x fire/*") != std::string::npos);
    assert(rebase.find("tools/install-help-command.sh") != std::string::npos);
    assert(rebase.find(".escape-room-help.sh") != std::string::npos);

    assert(installHelp.find("help()") != std::string::npos);
    assert(installHelp.find(".bashrc") != std::string::npos);
    assert(installHelp.find("cd escape-room") != std::string::npos);

    assert(help.find("cd escape-room") != std::string::npos);
    assert(help.find("tools/capture-fire-panel-buttons.sh") != std::string::npos);

    assert(capture.find("fire-panel-button-order") != std::string::npos);
    assert(capture.find("Press the button labeled STATUS") != std::string::npos);
    assert(capture.find("Press the button labeled RESET-ALL") != std::string::npos);
    assert(capture.find("send this file") != std::string::npos);

    const std::vector<std::string> fireCommands = {
        "status",
        "film-on",
        "film-off",
        "sound-look",
        "sound-crash",
        "sound-fail",
        "sound-pass",
        "sound-bake",
        "unlock",
        "reset-all",
    };

    for (const std::string& command : fireCommands) {
        assert(capture.find("escape/fire/" + command) != std::string::npos);
        assert(pico7.find("{\"" + command + "\", EscapeTopic::") != std::string::npos);
        assert(controller.find("FIRE_") != std::string::npos);
        assert(readText("fire/" + command).find("\"" + command + "\"") != std::string::npos);
    }
}
