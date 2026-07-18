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

    assert(text.find("ffplay -nodisp -autoexit -loglevel quiet") != std::string::npos);
    assert(text.find("-fflags nobuffer") != std::string::npos);
    assert(text.find("-flags low_delay") != std::string::npos);
    assert(text.find("-probesize 32") != std::string::npos);
    assert(text.find("-analyzeduration 0") != std::string::npos);
    assert(text.find("std::condition_variable") != std::string::npos);
    assert(text.find("std::deque<AudioRequest>") != std::string::npos);
    assert(text.find("audioWorkerLoop") != std::string::npos);
    assert(text.find("std::thread(audioWorkerLoop).detach()") != std::string::npos);
    assert(text.find("enqueueAudio") != std::string::npos);
    assert(text.find("Queued audio:") != std::string::npos);
    assert(text.find("cmd += \" &\"") == std::string::npos);
    assert(text.find("playInBackground") == std::string::npos);

    assert(piMain.find("yeahYouDidItAudio(get_project_asset_file(\"yeah-you-did-it.mp3\"), false)") == std::string::npos);
    assert(piMain.find("bakeAt350Audio(get_project_asset_file(\"bake_at_350.wav\"), false)") == std::string::npos);

    return 0;
}
