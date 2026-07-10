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
    const std::string main = readText("raspberry-pi-controller/src/main.cpp");
    const std::string controller = readText("raspberry-pi-controller/src/GameController.cpp");
    const std::string controllerHeader = readText("raspberry-pi-controller/src/GameController.h");

    const char* cueFiles[] = {
        "beep-boop-reset-complete.wav",
        "beep-beep-boop-boop-reset-complete.wav",
        "resetedededed.wav",
        "lets-get-to-bakin&escapin.wav",
        "let-us-bake&escape-if-you-can.wav",
        "escape-room-activated-lets-go.wav",
        "its-time-for-some-baking-and-escaping-lets-go.wav",
    };

    for (const char* cueFile : cueFiles) {
        assert(main.find(cueFile) != std::string::npos);
    }

    assert(main.find("RandomEffect roomCueAudio") != std::string::npos);
    assert(main.find("triggerRoomCue(\"room activated\")") != std::string::npos);
    assert(main.find("triggerRoomCue(\"room reset\")") != std::string::npos);
    assert(main.find("publish_reset_topic(mosq, EscapeTopic::RESET_PUZZLE") != std::string::npos);
    assert(main.find("publish_reset_topic(mosq, RESET_TOPIC") != std::string::npos);
    assert(controllerHeader.find("triggerRoomCue") != std::string::npos);
    assert(controller.find("roomCueEffect->trigger(payload)") != std::string::npos);
    assert(controller.find("triggerRoomCue(\"room reset\")") != std::string::npos);

    return 0;
}
