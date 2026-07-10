#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream source("pico2-copper-final-piece/src/main.cpp");
    assert(source.good());

    std::ostringstream buffer;
    buffer << source.rdbuf();
    std::string text = buffer.str();

    assert(text.find("FINAL_PIECE_PIN") == std::string::npos);
    assert(text.find("FINAL_PIECE_PLACED") == std::string::npos);
    assert(text.find("final_piece") == std::string::npos);
    assert(text.find("COPPER_COMPLETE_PIN") != std::string::npos);
    assert(text.find("COPPER_PUZZLE_COMPLETE") != std::string::npos);
    assert(text.find("needsPhysicalReset") != std::string::npos);
    assert(text.find("copperPuzzle.needsPhysicalReset = state == PUZZLE_ACTIVE_STATE") != std::string::npos);
    assert(text.find("if (copperPuzzle.needsPhysicalReset)") != std::string::npos);
    assert(text.find("postStatePayload(copperPuzzle.solved)") != std::string::npos);
}
