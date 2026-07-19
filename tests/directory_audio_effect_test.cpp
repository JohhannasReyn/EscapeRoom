#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../raspberry-pi-controller/src/effects/DirectoryAudioEffect.h"

int main() {
    namespace fs = std::filesystem;

    fs::path testDir = fs::temp_directory_path() / "escape-room-directory-audio-effect-test";
    fs::remove_all(testDir);
    fs::create_directories(testDir);

    const std::vector<std::string> fileNames = {
        "z-last.mp3",
        "a-first.wav",
        "middle.m4a",
        "video-audio.MP4",
        "ignore.txt",
    };

    for (const std::string& fileName : fileNames) {
        std::ofstream file(testDir / fileName);
        file << "test";
    }

    DirectoryAudioEffect effect(testDir.string());
    std::vector<std::string> files = effect.files();

    assert(files.size() == 4);
    assert(fs::path(files[0]).filename().string() == "a-first.wav");
    assert(fs::path(files[1]).filename().string() == "middle.m4a");
    assert(fs::path(files[2]).filename().string() == "video-audio.MP4");
    assert(fs::path(files[3]).filename().string() == "z-last.mp3");
    assert(DirectoryAudioEffect::isSupportedAudioFile("clip.mp4") == true);
    assert(DirectoryAudioEffect::isSupportedAudioFile("clip.MP3") == true);
    assert(DirectoryAudioEffect::isSupportedAudioFile("notes.txt") == false);

    DirectoryAudioEffect missing((testDir / "missing").string());
    assert(missing.files().empty());

    fs::remove_all(testDir);
    return 0;
}
