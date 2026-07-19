#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream source("raspberry-pi-controller/src/effects/AudioEffect.cpp");
    assert(source.good());

    std::ostringstream buffer;
    buffer << source.rdbuf();
    const std::string text = buffer.str();

    std::ifstream piMainSource("raspberry-pi-controller/src/main.cpp");
    assert(piMainSource.good());

    std::ostringstream piMainBuffer;
    piMainBuffer << piMainSource.rdbuf();
    const std::string piMain = piMainBuffer.str();

    assert(text.find("timeout --kill-after=2s 15s") != std::string::npos);
    assert(text.find("bash -o pipefail -c") != std::string::npos);
    assert(text.find("ffmpeg -hide_banner -loglevel error -nostdin") != std::string::npos);
    assert(text.find("-i \\\"$1\\\" -f wav -") != std::string::npos);
    assert(text.find("aplay -q -D \\\"$2\\\" -") != std::string::npos);
    assert(text.find("audioCommandForFile") != std::string::npos);
    assert(text.find("ffplay") == std::string::npos);
    assert(text.find("std::condition_variable") != std::string::npos);
    assert(text.find("std::deque<AudioRequest>") != std::string::npos);
    assert(text.find("audioWorkerLoop") != std::string::npos);
    assert(text.find("std::thread(audioWorkerLoop).detach()") != std::string::npos);
    assert(text.find("enqueueAudio") != std::string::npos);
    assert(text.find("Queued audio:") != std::string::npos);
    assert(text.find("FAIL_SAFE audio playback failed") != std::string::npos);
    assert(text.find("Retrying audio through fallback device") != std::string::npos);
    assert(text.find("Audio playback command completed") != std::string::npos);
    assert(text.find("cmd += \" &\"") == std::string::npos);
    assert(text.find("playInBackground") == std::string::npos);

    assert(piMain.find("yeahYouDidItAudio(get_project_asset_file(\"yeah-you-did-it.mp3\"), false)") == std::string::npos);
    assert(piMain.find("bakeAt350Audio(get_project_asset_file(\"bake_at_350.wav\"), false)") == std::string::npos);

    return 0;
}
