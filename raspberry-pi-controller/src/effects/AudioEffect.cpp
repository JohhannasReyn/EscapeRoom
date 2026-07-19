#include "AudioEffect.h"

#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

namespace {
struct AudioRequest {
    std::string command;
    std::string fallbackCommand;
    std::string file;
    std::string payload;
};

struct AudioPlaybackQueue {
    std::mutex mutex;
    std::condition_variable ready;
    std::deque<AudioRequest> requests;
};

AudioPlaybackQueue& playbackQueue() {
    // The detached worker runs until process exit, so keep its queue alive for
    // the whole process lifetime instead of relying on static destructor order.
    static AudioPlaybackQueue* queue = new AudioPlaybackQueue();
    return *queue;
}

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

std::string audioCommandForFileAndDevice(const std::string& audioFile, const std::string& device) {
    const std::string decodeAndPlay =
        "ffmpeg -hide_banner -loglevel error -nostdin -i \"$1\" -f wav - | "
        "aplay -q -D \"$2\" -";

    return "timeout --kill-after=2s 15s bash -o pipefail -c " +
        shellQuote(decodeAndPlay) +
        " _ " +
        shellQuote(audioFile) +
        " " +
        shellQuote(device);
}

std::string audioCommandForFile(const std::string& audioFile) {
    return audioCommandForFileAndDevice(audioFile, audioDevice());
}

std::string audioFallbackCommandForFile(const std::string& audioFile) {
    if (audioDevice() == "default") {
        return "";
    }

    return audioCommandForFileAndDevice(audioFile, "default");
}

void audioWorkerLoop() {
    AudioPlaybackQueue& queue = playbackQueue();

    while (true) {
        AudioRequest request;

        {
            std::unique_lock<std::mutex> lock(queue.mutex);
            queue.ready.wait(lock, [&queue]() {
                return !queue.requests.empty();
            });
            request = std::move(queue.requests.front());
            queue.requests.pop_front();
        }

        std::cout << "Playing queued audio: " << request.file << std::endl;
        std::cout << "Audio command: " << request.command << std::endl;
        int result = std::system(request.command.c_str());

        if (result == 0) {
            std::cout << "Audio playback command completed: " << request.file << std::endl;
            continue;
        }

        std::cout << "FAIL_SAFE audio playback failed for " << request.file
                  << " with result " << result << "." << std::endl;

        if (!request.fallbackCommand.empty()) {
            std::cout << "Retrying audio through fallback device: default" << std::endl;
            int fallbackResult = std::system(request.fallbackCommand.c_str());

            if (fallbackResult == 0) {
                std::cout << "Audio playback command completed through fallback device: " << request.file << std::endl;
                continue;
            }

            std::cout << "FAIL_SAFE audio fallback failed for " << request.file
                      << " with result " << fallbackResult << "." << std::endl;
        }

        std::cout << "If no sound played, verify the 3.5mm speaker is powered, plugged in, and selected as the Pi audio output." << std::endl;
    }
}

void ensureAudioWorkerStarted() {
    static std::once_flag audioWorkerStarted;
    std::call_once(audioWorkerStarted, []() {
        std::thread(audioWorkerLoop).detach();
    });
}

void enqueueAudio(AudioRequest request) {
    ensureAudioWorkerStarted();

    AudioPlaybackQueue& queue = playbackQueue();
    {
        std::lock_guard<std::mutex> lock(queue.mutex);
        queue.requests.push_back(std::move(request));
    }

    queue.ready.notify_one();
}
}

AudioEffect::AudioEffect(std::string audioFile, bool)
    : audioFile(std::move(audioFile)) {
}

void AudioEffect::trigger(const std::string& payload) {
    std::cout << "Payload: " << payload << std::endl;

    std::ifstream file(audioFile);
    if (!file.good()) {
        std::cout << "FAIL_SAFE audio file missing: " << audioFile << std::endl;
        return;
    }

    enqueueAudio({audioCommandForFile(audioFile), audioFallbackCommandForFile(audioFile), audioFile, payload});
    std::cout << "Queued audio: " << audioFile << std::endl;
}

std::string AudioEffect::file() const {
    return audioFile;
}
