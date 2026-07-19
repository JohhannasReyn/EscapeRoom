#include "DirectoryAudioEffect.h"

#include "AudioEffect.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <utility>

DirectoryAudioEffect::DirectoryAudioEffect(std::string audioDirectory)
    : audioDirectory(std::move(audioDirectory)) {
}

bool DirectoryAudioEffect::isSupportedAudioFile(const std::string& path) {
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return extension == ".wav" ||
        extension == ".mp3" ||
        extension == ".m4a" ||
        extension == ".mp4" ||
        extension == ".aac" ||
        extension == ".flac" ||
        extension == ".ogg";
}

std::vector<std::string> DirectoryAudioEffect::files() const {
    namespace fs = std::filesystem;
    std::vector<std::string> audioFiles;

    std::error_code error;
    if (!fs::is_directory(audioDirectory, error)) {
        return audioFiles;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(audioDirectory, error)) {
        if (error) {
            break;
        }

        if (!entry.is_regular_file(error)) {
            error.clear();
            continue;
        }

        std::string path = entry.path().string();
        if (isSupportedAudioFile(path)) {
            audioFiles.push_back(path);
        }
    }

    std::sort(audioFiles.begin(), audioFiles.end());
    return audioFiles;
}

void DirectoryAudioEffect::trigger(const std::string& payload) {
    std::vector<std::string> audioFiles = files();

    if (audioFiles.empty()) {
        std::cout << "Audio play-all skipped: no supported audio files in " << audioDirectory << std::endl;
        return;
    }

    std::cout << "Queueing " << audioFiles.size() << " audio file(s) from " << audioDirectory << std::endl;
    for (const std::string& audioFile : audioFiles) {
        AudioEffect(audioFile).trigger(payload);
    }
}
