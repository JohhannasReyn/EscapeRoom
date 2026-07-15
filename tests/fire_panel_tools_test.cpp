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
    const std::string send = readText("tools/send_to_john.sh");
    const std::string setupDrive = readText("tools/setup-drive-upload.sh");
    const std::string testPico4 = readText("tools/test-pico4.sh");
    const std::string testPico5 = readText("tools/test-pico5.sh");
    const std::string contactExample = readText("john-contact.env.example");
    const std::string pico7 = readText("pico7-fire-panel/src/main.cpp");
    const std::string controller = readText("raspberry-pi-controller/src/GameController.cpp");

    assert(rebase.find("chmod +x tools/*.sh") != std::string::npos);
    assert(rebase.find("chmod +x fire/*") != std::string::npos);
    assert(rebase.find("tools/install-help-command.sh") != std::string::npos);
    assert(rebase.find(".escape-room-help.sh") != std::string::npos);
    assert(rebase.find("john-contact.env.example") != std::string::npos);

    assert(installHelp.find("help()") != std::string::npos);
    assert(installHelp.find(".bashrc") != std::string::npos);
    assert(installHelp.find("cd escape-room") != std::string::npos);

    assert(help.find("cd escape-room") != std::string::npos);
    assert(help.find("tools/capture-fire-panel-buttons.sh") != std::string::npos);
    assert(help.find("tools/send_to_john.sh") != std::string::npos);
    assert(help.find("tools/setup-drive-upload.sh") != std::string::npos);
    assert(help.find("do not run it with source") != std::string::npos);
    assert(help.find("Fire panel button map") != std::string::npos);
    assert(help.find("| STATUS | GP2 | escape/fire/status |") != std::string::npos);
    assert(help.find("| RESET-ALL | GP11 | escape/fire/reset-all |") != std::string::npos);
    assert(help.find("Light state key") != std::string::npos);
    assert(help.find("Alternating red/green") != std::string::npos);

    const std::string readme = readText("README.md");
    assert(readme.find("tools/rebase.sh") != std::string::npos);
    assert(readme.find("tools/flash-pico.sh all") != std::string::npos);
    assert(readme.find("tools/setup-room.sh") != std::string::npos);
    assert(readme.find("do not\nrun it with `source`") != std::string::npos);

    assert(testPico4.find("escape/cmd/pico4/reveal_smart_film") != std::string::npos);
    assert(testPico4.find("escape/pico4/smart_film_ready") != std::string::npos);
    assert(testPico4.find("transparent") != std::string::npos);
    assert(testPico4.find("Pico 4 acknowledged smart film") != std::string::npos);
    assert(testPico4.find("escape/telemetry/pico4/oven") != std::string::npos);
    assert(testPico4.find("Turn the oven knob") != std::string::npos);
    assert(testPico4.find("oven_raw range") != std::string::npos);

    assert(testPico5.find("escape/pico5/color_sequence_error") != std::string::npos);
    assert(testPico5.find("try-again.wav") != std::string::npos);
    assert(testPico5.find("buzzer.mp3") != std::string::npos);
    assert(testPico5.find("escape/telemetry/pico5/buttons") != std::string::npos);

    assert(capture.find("fire-panel-button-order") != std::string::npos);
    assert(capture.find("Press the button labeled STATUS") != std::string::npos);
    assert(capture.find("Press the button labeled RESET-ALL") != std::string::npos);
    assert(capture.find("send this file") != std::string::npos);

    assert(send.find("fire-panel-button-order-*.txt") != std::string::npos);
    assert(send.find("JOHN_RCLONE_TARGET") != std::string::npos);
    assert(send.find("JOHN_RCLONE_ROOT_FOLDER_ID") != std::string::npos);
    assert(send.find("rclone copy") != std::string::npos);
    assert(send.find("JOHN_UPLOAD_URL") != std::string::npos);
    assert(send.find("JOHN_SCP_TARGET") != std::string::npos);
    assert(send.find("JOHN_EMAIL") != std::string::npos);
    assert(send.find("for-john.tar.gz") != std::string::npos);
    assert(send.find("john-contact.env.example") != std::string::npos);
    assert(send.find("prompt_for_email_if_needed") != std::string::npos);
    assert(send.find("Email address, or press Enter to skip") != std::string::npos);
    assert(send.find("save_email_config") != std::string::npos);
    assert(send.find("Saved email for future runs") != std::string::npos);

    assert(contactExample.find("JOHN_UPLOAD_URL") != std::string::npos);
    assert(contactExample.find("JOHN_RCLONE_TARGET") != std::string::npos);
    assert(contactExample.find("JOHN_RCLONE_ROOT_FOLDER_ID") != std::string::npos);
    assert(contactExample.find("1QYtv2RmXZq8g6iEgjdxn2W2cG39L_TYW") != std::string::npos);
    assert(contactExample.find("JOHN_SCP_TARGET") != std::string::npos);
    assert(contactExample.find("JOHN_EMAIL") != std::string::npos);

    assert(setupDrive.find("JOHN_RCLONE_TARGET") != std::string::npos);
    assert(setupDrive.find("DEFAULT_REMOTE=\"escape-room-drive\"") != std::string::npos);
    assert(setupDrive.find("1QYtv2RmXZq8g6iEgjdxn2W2cG39L_TYW") != std::string::npos);
    assert(pico7.find("AlternatingRedGreen") != std::string::npos);
    assert(pico7.find("alternating-red-green") != std::string::npos);

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
