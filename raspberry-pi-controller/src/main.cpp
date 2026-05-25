#include <mosquitto.h>
#include <gpiod.h>

#include "GameController.h"
#include "ResetControl.h"
#include "effects/AudioEffect.h"
#include "puzzles/CopperPuzzle.h"
#include "puzzles/PlannedPuzzles.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

static const char* MQTT_HOST = "localhost";
static const int MQTT_PORT = 1883;
static const char* CLIENT_ID = "raspberry_pi_game_controller";
static const char* GPIO_CHIP_NAME = "gpiochip0";
static const int POST_ALL_GREEN_MS = 1200;

std::string get_home_dir() {
    const char* home = std::getenv("HOME");

    if (home == nullptr) {
        return "/home/pi";
    }

    return std::string(home);
}

std::string get_audio_file() {
    return get_home_dir() + "/escape-room/crash.wav";
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

void watch_reset_button(struct mosquitto* mosq, GameController& controller, std::atomic<bool>& running) {
    gpiod_chip* chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);

    if (chip == nullptr) {
        std::cout << "Reset button disabled: could not open GPIO chip " << GPIO_CHIP_NAME << std::endl;
        return;
    }

    gpiod_line* line = gpiod_chip_get_line(chip, RESET_BUTTON_GPIO);

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

        std::this_thread::sleep_for(std::chrono::milliseconds(RESET_POLL_MS));
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
}

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

    std::cout << std::endl;
    std::cout << "MQTT message received." << std::endl;
    std::cout << "Topic: " << topic << std::endl;
    std::cout << "Payload: " << payload << std::endl;

    auto* controller = static_cast<GameController*>(userdata);

    if (controller != nullptr) {
        controller->handleMessage(topic, payload);
        publish_pending_commands(mosq, *controller);
    }
}

int main() {
    AudioEffect crashAudio(get_audio_file());

    GameController controller;
    controller.addPuzzle(std::make_unique<CopperPuzzle>(crashAudio));
    controller.addPuzzle(std::make_unique<StairsPuzzle>());
    controller.addPuzzle(std::make_unique<DowelsPuzzle>());
    controller.addPuzzle(std::make_unique<WinePuzzle>());
    controller.addPuzzle(std::make_unique<BlenderPuzzle>());
    controller.addPuzzle(std::make_unique<FireplacePuzzle>());
    controller.addPuzzle(std::make_unique<PhonePuzzle>());
    controller.addPuzzle(std::make_unique<WindowPuzzle>());
    controller.addPuzzle(std::make_unique<OvenPuzzle>());

    std::cout << "Escape Room Raspberry Pi Game Controller" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Broker: " << MQTT_HOST << ":" << MQTT_PORT << std::endl;
    std::cout << "Audio:  " << get_audio_file() << std::endl;
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
