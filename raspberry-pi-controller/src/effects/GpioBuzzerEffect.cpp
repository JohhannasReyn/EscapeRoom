#include "GpioBuzzerEffect.h"

#include <gpiod.h>

#include <chrono>
#include <iostream>
#include <thread>

static const char* GPIO_CHIP_NAME = "gpiochip0";
static const char* GPIO_CHIP_PATH = "/dev/gpiochip0";

GpioBuzzerEffect::GpioBuzzerEffect(int gpioPin, int durationMs)
    : gpioPin(gpioPin), durationMs(durationMs) {
}

void GpioBuzzerEffect::trigger(const std::string& payload) {
    std::cout << "Pi buzzer trigger. GPIO " << gpioPin << ", payload: " << payload << std::endl;

    if (durationMs <= 0) {
        std::cout << "Pi buzzer skipped: duration must be positive." << std::endl;
        return;
    }

#if defined(LIBGPIOD_V2) || (defined(GPIOD_VERSION_MAJOR) && GPIOD_VERSION_MAJOR >= 2)
    struct gpiod_chip* chip = gpiod_chip_open(GPIO_CHIP_PATH);

    if (chip == nullptr) {
        std::cout << "Pi buzzer disabled: could not open GPIO chip " << GPIO_CHIP_PATH << "." << std::endl;
        return;
    }

    struct gpiod_line_settings* settings = gpiod_line_settings_new();
    struct gpiod_line_config* lineConfig = gpiod_line_config_new();
    struct gpiod_request_config* requestConfig = gpiod_request_config_new();

    if (settings == nullptr || lineConfig == nullptr || requestConfig == nullptr) {
        std::cout << "Pi buzzer disabled: could not allocate GPIO request config." << std::endl;
        if (settings != nullptr) {
            gpiod_line_settings_free(settings);
        }
        if (lineConfig != nullptr) {
            gpiod_line_config_free(lineConfig);
        }
        if (requestConfig != nullptr) {
            gpiod_request_config_free(requestConfig);
        }
        gpiod_chip_close(chip);
        return;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    unsigned int buzzerLineOffset = static_cast<unsigned int>(gpioPin);
    int addLineRc = gpiod_line_config_add_line_settings(lineConfig, &buzzerLineOffset, 1, settings);

    if (addLineRc != 0) {
        std::cout << "Pi buzzer disabled: could not configure GPIO " << gpioPin << "." << std::endl;
        gpiod_request_config_free(requestConfig);
        gpiod_line_config_free(lineConfig);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return;
    }

    gpiod_request_config_set_consumer(requestConfig, "escape-room-bake-buzzer");
    struct gpiod_line_request* request = gpiod_chip_request_lines(chip, requestConfig, lineConfig);

    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(settings);

    if (request == nullptr) {
        std::cout << "Pi buzzer disabled: could not request GPIO " << gpioPin << " as output." << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    gpiod_line_request_set_value(request, gpioPin, GPIOD_LINE_VALUE_ACTIVE);
    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    gpiod_line_request_set_value(request, gpioPin, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
#else
    struct gpiod_chip* chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);

    if (chip == nullptr) {
        std::cout << "Pi buzzer disabled: could not open GPIO chip " << GPIO_CHIP_NAME << "." << std::endl;
        return;
    }

    struct gpiod_line* line = gpiod_chip_get_line(chip, gpioPin);

    if (line == nullptr) {
        std::cout << "Pi buzzer disabled: could not open GPIO " << gpioPin << "." << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    int requestRc = gpiod_line_request_output(line, "escape-room-bake-buzzer", 0);

    if (requestRc != 0) {
        std::cout << "Pi buzzer disabled: could not request GPIO " << gpioPin << " as output." << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    gpiod_line_set_value(line, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    gpiod_line_set_value(line, 0);

    gpiod_line_release(line);
    gpiod_chip_close(chip);
#endif
}
