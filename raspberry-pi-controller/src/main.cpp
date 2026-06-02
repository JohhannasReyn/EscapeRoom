#include <mosquitto.h>
#include <gpiod.h>

#include "GameController.h"
#include "ResetControl.h"
#include "effects/AudioEffect.h"
#include "effects/DisplayOutput.h"
#include "effects/GpioBuzzerEffect.h"
#include "puzzles/CopperPuzzle.h"
#include "puzzles/PlannedPuzzles.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

static const char* MQTT_HOST = "localhost";
static const int MQTT_PORT = 1883;
static const char* CLIENT_ID = "raspberry_pi_game_controller";
static const char* GPIO_CHIP_NAME = "gpiochip0";
static const char* GPIO_CHIP_PATH = "/dev/gpiochip0";
static const int POST_ALL_GREEN_MS = 1200;

#ifndef PI_BAKE_BUZZER_GPIO
#define PI_BAKE_BUZZER_GPIO 24
#endif

#ifndef PI_BAKE_BUZZER_MS
#define PI_BAKE_BUZZER_MS 350
#endif

std::string get_home_dir() {
    const char* home = std::getenv("HOME");

    if (home == nullptr) {
        return "/home/pi";
    }

    return std::string(home);
}

std::string get_project_asset_file(const std::string& fileName) {
    const std::string candidates[] = {
        get_home_dir() + "/escape-room/assets/audio/" + fileName,
        "/home/admin/escape-room/assets/audio/" + fileName,
        "./assets/audio/" + fileName,
        "../assets/audio/" + fileName,
        get_home_dir() + "/escape-room/assets/" + fileName,
        "/home/admin/escape-room/assets/" + fileName,
        "./assets/" + fileName,
        "../assets/" + fileName
    };

    for (const auto& candidate : candidates) {
        std::ifstream assetFile(candidate);

        if (assetFile.good()) {
            return candidate;
        }
    }

    return get_home_dir() + "/escape-room/assets/audio/" + fileName;
}

void publish_reset(struct mosquitto* mosq) {
    const char* payload = "reset";

    int rc = mosquitto_publish(
        mosq,
        nullptr,
        RESET_TOPIC,
        static_cast<int>(std::strlen(payload)),
        payload,
        0,
        false
    );

    if (rc == MOSQ_ERR_SUCCESS) {
        std::cout << "Published reset command: " << RESET_TOPIC << std::endl;
    } else {
        std::cout << "Reset publish failed: " << mosquitto_strerror(rc) << std::endl;
    }
}

void publish_pending_commands(struct mosquitto* mosq, GameController& controller, int delayAfterEachMs = 0) {
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();

        if (command.topic == "escape/cubby/all/status" && command.payload == "off") {
            std::this_thread::sleep_for(std::chrono::milliseconds(POST_ALL_GREEN_MS));
        }

        int rc = mosquitto_publish(
            mosq,
            nullptr,
            command.topic.c_str(),
            static_cast<int>(command.payload.size()),
            command.payload.c_str(),
            0,
            false
        );

        if (rc == MOSQ_ERR_SUCCESS) {
            std::cout << "Published command: " << command.topic << " -> " << command.payload << std::endl;
        } else {
            std::cout << "Command publish failed for " << command.topic << ": " << mosquitto_strerror(rc) << std::endl;
        }

        if (delayAfterEachMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayAfterEachMs));
        }
    }
}

void handle_reset_button_value(
    int value,
    unsigned long& heldMs,
    bool& resetPublishedForPress,
    struct mosquitto* mosq,
    GameController& controller
) {
    bool pressed = value == 0;

    if (pressed) {
        heldMs += RESET_POLL_MS;

        if (!resetPublishedForPress && resetPressReady(heldMs, pressed)) {
            publish_reset(mosq);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            controller.queuePostQueryCommand();
            publish_pending_commands(mosq, controller);
            resetPublishedForPress = true;
        }
    } else {
        heldMs = 0;
        resetPublishedForPress = false;
    }
}

#if defined(LIBGPIOD_V2) || (defined(GPIOD_VERSION_MAJOR) && GPIOD_VERSION_MAJOR >= 2)
void watch_reset_button(struct mosquitto* mosq, GameController& controller, std::atomic<bool>& running) {
    struct gpiod_chip* chip = gpiod_chip_open(GPIO_CHIP_PATH);

    if (chip == nullptr) {
        std::cout << "Reset button disabled: could not open GPIO chip " << GPIO_CHIP_PATH << std::endl;
        return;
    }

    struct gpiod_line_settings* settings = gpiod_line_settings_new();
    struct gpiod_line_config* lineConfig = gpiod_line_config_new();
    struct gpiod_request_config* requestConfig = gpiod_request_config_new();

    if (settings == nullptr || lineConfig == nullptr || requestConfig == nullptr) {
        std::cout << "Reset button disabled: could not allocate GPIO request config." << std::endl;
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

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    unsigned int resetLineOffset = RESET_BUTTON_GPIO;
    int addLineRc = gpiod_line_config_add_line_settings(lineConfig, &resetLineOffset, 1, settings);

    if (addLineRc != 0) {
        std::cout << "Reset button disabled: could not configure GPIO " << RESET_BUTTON_GPIO << "." << std::endl;
        gpiod_request_config_free(requestConfig);
        gpiod_line_config_free(lineConfig);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return;
    }

    gpiod_request_config_set_consumer(requestConfig, "escape-room-reset-button");
    struct gpiod_line_request* request = gpiod_chip_request_lines(chip, requestConfig, lineConfig);

    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(settings);

    if (request == nullptr) {
        std::cout << "Reset button disabled: could not request GPIO " << RESET_BUTTON_GPIO << " as input." << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    std::cout << "Reset button enabled on Raspberry Pi GPIO " << RESET_BUTTON_GPIO << "." << std::endl;
    std::cout << "Hold for " << RESET_HOLD_MS << " ms to publish " << RESET_TOPIC << "." << std::endl;

    bool resetPublishedForPress = false;
    unsigned long heldMs = 0;

    while (running.load()) {
        enum gpiod_line_value value = gpiod_line_request_get_value(request, RESET_BUTTON_GPIO);

        if (value == GPIOD_LINE_VALUE_ERROR) {
            std::cout << "Reset button read failed." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(RESET_POLL_MS));
            continue;
        }

        handle_reset_button_value(
            value == GPIOD_LINE_VALUE_INACTIVE ? 0 : 1,
            heldMs,
            resetPublishedForPress,
            mosq,
            controller
        );

        std::this_thread::sleep_for(std::chrono::milliseconds(RESET_POLL_MS));
    }

    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
}
#else
void watch_reset_button(struct mosquitto* mosq, GameController& controller, std::atomic<bool>& running) {
    struct gpiod_chip* chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);

    if (chip == nullptr) {
        std::cout << "Reset button disabled: could not open GPIO chip " << GPIO_CHIP_NAME << std::endl;
        return;
    }

    struct gpiod_line* line = gpiod_chip_get_line(chip, RESET_BUTTON_GPIO);

    if (line == nullptr) {
        std::cout << "Reset button disabled: could not open GPIO " << RESET_BUTTON_GPIO << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    int request_rc = gpiod_line_request_input_flags(
        line,
        "escape-room-reset-button",
        GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP
    );

    if (request_rc != 0) {
        std::cout << "Reset button disabled: could not request GPIO " << RESET_BUTTON_GPIO << " as input." << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    std::cout << "Reset button enabled on Raspberry Pi GPIO " << RESET_BUTTON_GPIO << "." << std::endl;
    std::cout << "Hold for " << RESET_HOLD_MS << " ms to publish " << RESET_TOPIC << "." << std::endl;

    bool resetPublishedForPress = false;
    unsigned long heldMs = 0;

    while (running.load()) {
        int value = gpiod_line_get_value(line);

        if (value < 0) {
            std::cout << "Reset button read failed." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(RESET_POLL_MS));
            continue;
        }

        handle_reset_button_value(value, heldMs, resetPublishedForPress, mosq, controller);

        std::this_thread::sleep_for(std::chrono::milliseconds(RESET_POLL_MS));
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
}
#endif

void on_connect(struct mosquitto* mosq, void* userdata, int rc) {
    if (rc == 0) {
        std::cout << "Connected to MQTT broker." << std::endl;

        auto* controller = static_cast<GameController*>(userdata);

        if (controller == nullptr) {
            std::cout << "No game controller attached; cannot subscribe to puzzle topics." << std::endl;
            return;
        }

        for (const auto& topic : controller->topics()) {
            int sub_rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 0);

            if (sub_rc == MOSQ_ERR_SUCCESS) {
                std::cout << "Subscribed to topic: " << topic << std::endl;
            } else {
                std::cout << "Subscribe failed for topic " << topic << ". Error code: " << sub_rc << std::endl;
            }
        }

        int post_rc = mosquitto_subscribe(mosq, nullptr, "escape/post/cubby/+/state", 0);

        if (post_rc == MOSQ_ERR_SUCCESS) {
            std::cout << "Subscribed to topic: escape/post/cubby/+/state" << std::endl;
        } else {
            std::cout << "Subscribe failed for POST state reports. Error code: " << post_rc << std::endl;
        }

        int oven_degrees_rc = mosquitto_subscribe(mosq, nullptr, "escape/oven/degrees", 0);

        if (oven_degrees_rc == MOSQ_ERR_SUCCESS) {
            std::cout << "Subscribed to topic: escape/oven/degrees" << std::endl;
        } else {
            std::cout << "Subscribe failed for oven dial degrees. Error code: " << oven_degrees_rc << std::endl;
        }

        int telemetry_rc = mosquitto_subscribe(mosq, nullptr, EscapeTopic::SENSOR_TELEMETRY_WILDCARD, 0);

        if (telemetry_rc == MOSQ_ERR_SUCCESS) {
            std::cout << "Subscribed to topic: " << EscapeTopic::SENSOR_TELEMETRY_WILDCARD << std::endl;
        } else {
            std::cout << "Subscribe failed for sensor telemetry. Error code: " << telemetry_rc << std::endl;
        }

        controller->queuePostQueryCommand();
        publish_pending_commands(mosq, *controller);
    } else {
        std::cout << "MQTT connection failed. Code: " << rc << std::endl;
    }
}

void on_disconnect(struct mosquitto* mosq, void* userdata, int rc) {
    std::cout << "Disconnected from MQTT broker. Code: " << rc << std::endl;

    if (rc != 0) {
        std::cout << "Unexpected disconnect. Mosquitto will attempt reconnect." << std::endl;
    }
}

void on_message(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* msg) {
    if (msg == nullptr || msg->topic == nullptr) {
        return;
    }

    std::string topic = msg->topic;
    std::string payload;

    if (msg->payload != nullptr && msg->payloadlen > 0) {
        payload.assign(static_cast<char*>(msg->payload), msg->payloadlen);
    }

    bool isTelemetry = topic.rfind("escape/telemetry/", 0) == 0;

    std::cout << std::endl;
    std::cout << (isTelemetry ? "Sensor telemetry received." : "MQTT message received.") << std::endl;
    std::cout << "Topic: " << topic << std::endl;
    std::cout << "Payload: " << payload << std::endl;

    if (isTelemetry) {
        return;
    }

    auto* controller = static_cast<GameController*>(userdata);

    if (controller != nullptr) {
        controller->handleMessage(topic, payload);
        publish_pending_commands(mosq, *controller);
    }
}

int main() {
    AudioEffect crashingPlatesAudio(get_project_asset_file("crashing_plates.m4a"));
    AudioEffect wrongCodeAudio(get_project_asset_file("buzzer.mp3"));
    GpioBuzzerEffect bakeAttentionBuzzer(PI_BAKE_BUZZER_GPIO, PI_BAKE_BUZZER_MS);
    DisplayOutput display;

    GameController controller(&crashingPlatesAudio, &display, &wrongCodeAudio, &bakeAttentionBuzzer);
    controller.addPuzzle(std::make_unique<StairsPuzzle>());
    controller.addPuzzle(std::make_unique<CopperPuzzle>());
    controller.addPuzzle(std::make_unique<FinalPiecePuzzle>());
    controller.addPuzzle(std::make_unique<PaintingRotationPuzzle>());
    controller.addPuzzle(std::make_unique<ColorButtonSequencePuzzle>());
    controller.addPuzzle(std::make_unique<ColorButtonSequenceErrorPuzzle>());
    controller.addPuzzle(std::make_unique<OvenTargetPuzzle>());
    controller.addPuzzle(std::make_unique<ElectromagUnlockedPuzzle>());

    std::cout << "Escape Room Raspberry Pi Game Controller" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Broker: " << MQTT_HOST << ":" << MQTT_PORT << std::endl;
    std::cout << "Painting audio: " << crashingPlatesAudio.file() << std::endl;
    std::cout << "Wrong-code audio: " << wrongCodeAudio.file() << std::endl;
    std::cout << "Bake attention buzzer: GPIO " << PI_BAKE_BUZZER_GPIO
              << " for " << PI_BAKE_BUZZER_MS << " ms" << std::endl;
    std::cout << "Registered puzzle topics:" << std::endl;
    for (const auto& topic : controller.topics()) {
        std::cout << "  " << topic << std::endl;
    }
    std::cout << std::endl;

    mosquitto_lib_init();

    mosquitto* mosq = mosquitto_new(CLIENT_ID, true, &controller);

    if (mosq == nullptr) {
        std::cerr << "Failed to create Mosquitto client." << std::endl;
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_threaded_set(mosq, true);

    mosquitto_reconnect_delay_set(
        mosq,
        1,      // minimum delay seconds
        10,     // maximum delay seconds
        true    // exponential backoff
    );

    int rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);

    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Could not connect to MQTT broker: " << mosquitto_strerror(rc) << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    std::cout << "Waiting for puzzle events..." << std::endl;

    std::atomic<bool> running(true);
    std::thread resetButtonThread(watch_reset_button, mosq, std::ref(controller), std::ref(running));

    rc = mosquitto_loop_forever(mosq, -1, 1);

    running.store(false);

    if (resetButtonThread.joinable()) {
        resetButtonThread.join();
    }

    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Mosquitto loop exited with error: " << mosquitto_strerror(rc) << std::endl;
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
