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

    assert(text.find("ffplay -nodisp -autoexit -loglevel quiet") != std::string::npos);
    assert(text.find("-fflags nobuffer") != std::string::npos);
    assert(text.find("-flags low_delay") != std::string::npos);
    assert(text.find("-probesize 32") != std::string::npos);
    assert(text.find("-analyzeduration 0") != std::string::npos);

    return 0;
}
