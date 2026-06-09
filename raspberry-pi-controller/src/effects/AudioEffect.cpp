#include "AudioEffect.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>

namespace {
std::string shellQuote(const std::string& value) {
    std::string quoted = "'";

    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }

    quoted += "'";
    return quoted;
}

std::string audioDevice() {
    const char* configuredDevice = std::getenv("ESCAPE_AUDIO_DEVICE");

    if (configuredDevice != nullptr && configuredDevice[0] != '\0') {
        return configuredDevice;
    }

    return "plughw:CARD=Headphones,DEV=0";
}
}

AudioEffect::AudioEffect(std::string audioFile, bool playInBackground)
    : audioFile(std::move(audioFile)), playInBackground(playInBackground) {
}

void AudioEffect::trigger(const std::string& payload) {
    std::cout << "Payload: " << payload << std::endl;
    std::cout << "Playing audio: " << audioFile << std::endl;

    std::ifstream file(audioFile);
    if (!file.good()) {
        std::cout << "Audio file missing: " << audioFile << std::endl;
        return;
    }

    std::string cmd;
    bool useFfplay =
        (audioFile.size() >= 4 && audioFile.substr(audioFile.size() - 4) == ".m4a") ||
        (audioFile.size() >= 4 && audioFile.substr(audioFile.size() - 4) == ".mp3");

    if (useFfplay) {
        cmd = "ffplay -nodisp -autoexit -loglevel quiet " + shellQuote(audioFile);
    } else {
        cmd = "aplay -D " + shellQuote(audioDevice()) + " " + shellQuote(audioFile);
    }

    if (playInBackground) {
        cmd += " &";
    }

    int result = std::system(cmd.c_str());

    if (result != 0) {
        std::cout << "Audio command returned non-zero result: " << result << std::endl;
        std::cout << "If no sound played, verify the 3.5mm speaker is powered, plugged in, and selected as the Pi audio output." << std::endl;
    }
}

std::string AudioEffect::file() const {
    return audioFile;
}
