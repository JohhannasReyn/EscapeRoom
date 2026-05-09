#include <mosquitto.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

static const char* MQTT_HOST = "localhost";
static const int MQTT_PORT = 1883;
static const char* MQTT_TOPIC = "escape/puzzle/copper/solved";
static const char* CLIENT_ID = "raspberry_pi_game_controller";

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

void play_sound() {
    std::string audio = get_audio_file();

    std::cout << "Playing crash sound: " << audio << std::endl;

    std::string cmd = "aplay \"" + audio + "\" &";
    int result = std::system(cmd.c_str());

    if (result != 0) {
        std::cout << "Audio command returned non-zero result: " << result << std::endl;
        std::cout << "If no sound played, verify the file exists and audio output is configured." << std::endl;
    }
}

void trigger_effect(const std::string& payload) {
    std::cout << std::endl;
    std::cout << "Puzzle solved event received!" << std::endl;
    std::cout << "Payload: " << payload << std::endl;

    play_sound();

    std::cout << "Effect complete." << std::endl;
}

void on_connect(struct mosquitto* mosq, void* userdata, int rc) {
    if (rc == 0) {
        std::cout << "Connected to MQTT broker." << std::endl;

        int sub_rc = mosquitto_subscribe(mosq, nullptr, MQTT_TOPIC, 0);

        if (sub_rc == MOSQ_ERR_SUCCESS) {
            std::cout << "Subscribed to topic: " << MQTT_TOPIC << std::endl;
        } else {
            std::cout << "Subscribe failed. Error code: " << sub_rc << std::endl;
        }
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

    if (topic == MQTT_TOPIC) {
        trigger_effect(payload);
    }
}

int main() {
    std::cout << "Escape Room Raspberry Pi Game Controller" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Broker: " << MQTT_HOST << ":" << MQTT_PORT << std::endl;
    std::cout << "Topic:  " << MQTT_TOPIC << std::endl;
    std::cout << "Audio:  " << get_audio_file() << std::endl;
    std::cout << std::endl;

    mosquitto_lib_init();

    mosquitto* mosq = mosquitto_new(CLIENT_ID, true, nullptr);

    if (mosq == nullptr) {
        std::cerr << "Failed to create Mosquitto client." << std::endl;
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);

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

    rc = mosquitto_loop_forever(mosq, -1, 1);

    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Mosquitto loop exited with error: " << mosquitto_strerror(rc) << std::endl;
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}