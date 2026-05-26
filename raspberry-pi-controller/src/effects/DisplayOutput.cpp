#include "DisplayOutput.h"

#include <chrono>
#include <iostream>
#include <thread>

void DisplayOutput::show_message(const std::string& text) {
    std::cout << "DISPLAY MESSAGE: " << text << std::endl;
}

void DisplayOutput::flash_message(const std::string& text, int durationSec, double intervalSec) {
    std::cout << "DISPLAY FLASH START: " << text << std::endl;

    if (durationSec <= 0 || intervalSec <= 0) {
        show_message(text);
        return;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(durationSec);
    auto interval = std::chrono::duration<double>(intervalSec);
    bool visible = true;

    while (std::chrono::steady_clock::now() < deadline) {
        if (visible) {
            show_message(text);
        } else {
            clear();
        }

        visible = !visible;
        std::this_thread::sleep_for(interval);
    }

    show_message(text);
    std::cout << "DISPLAY FLASH END: " << text << std::endl;
}

void DisplayOutput::clear() {
    std::cout << "DISPLAY CLEAR" << std::endl;
}
