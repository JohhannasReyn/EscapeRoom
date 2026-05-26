#include "AudioEffect.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>

AudioEffect::AudioEffect(std::string audioFile) : audioFile(std::move(audioFile)) {
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
    if (audioFile.size() >= 4 && audioFile.substr(audioFile.size() - 4) == ".m4a") {
        cmd = "ffplay -nodisp -autoexit -loglevel quiet \"" + audioFile + "\" &";
    } else {
        cmd = "aplay \"" + audioFile + "\" &";
    }

    int result = std::system(cmd.c_str());

    if (result != 0) {
        std::cout << "Audio command returned non-zero result: " << result << std::endl;
        std::cout << "If no sound played, verify the file exists and audio output is configured." << std::endl;
    }
}

std::string AudioEffect::file() const {
    return audioFile;
}
