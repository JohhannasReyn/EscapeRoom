#include <cassert>
#include <string>

#include "../shared/EscapeRoomProtocol.h"

int main() {
    assert(std::string(EscapeTopic::CUBBY_APPROACH_DETECTED) == "escape/pico1/cubby_approach_detected");
    assert(std::string(EscapeTopic::COPPER_PUZZLE_COMPLETE) == "escape/pico2/copper_puzzle_complete");
    assert(std::string(EscapeTopic::FINAL_PIECE_PLACED) == "escape/pico2/final_piece_placed");
    assert(std::string(EscapeTopic::PAINTING_ROTATION_COMPLETE) == "escape/pico3/painting_rotation_complete");
    assert(std::string(EscapeTopic::OVEN_POSITION_UPDATE) == "escape/pico4/oven_position_update");
    assert(std::string(EscapeTopic::OVEN_TARGET_REACHED) == "escape/pico4/oven_target_reached");
    assert(std::string(EscapeTopic::ELECTROMAG_LOCK_UNLOCKED) == "escape/pico4/electromag_lock_unlocked");
    assert(std::string(EscapeTopic::COLOR_SEQUENCE_COMPLETE) == "escape/pico5/color_sequence_complete");

    assert(std::string(EscapeTopic::ENABLE_CUBBY_LIGHT) == "escape/cmd/pico1/enable_cubby_light");
    assert(std::string(EscapeTopic::ENABLE_COPPER_PUZZLE) == "escape/cmd/pico2/enable_copper_puzzle");
    assert(std::string(EscapeTopic::ENABLE_PAINTING_ROTATION) == "escape/cmd/pico3/enable_painting_rotation");
    assert(std::string(EscapeTopic::REVEAL_SMART_FILM) == "escape/cmd/pico4/reveal_smart_film");
    assert(std::string(EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE) == "escape/cmd/pico5/enable_color_button_sequence");
    assert(std::string(EscapeTopic::ENABLE_OVEN_KNOB) == "escape/cmd/pico4/enable_oven_knob");
    assert(std::string(EscapeTopic::UNLOCK_ELECTROMAG_LOCK) == "escape/cmd/pico4/unlock_electromag_lock");
    assert(std::string(EscapeTopic::RESET_PUZZLE) == "escape/cmd/all/reset_puzzle");
    assert(std::string(EscapeTopic::STATUS_REQUEST) == "escape/cmd/all/status_request");
    assert(std::string(EscapeTopic::SENSOR_TELEMETRY_WILDCARD) == "escape/telemetry/#");
    assert(std::string(EscapeTopic::FIRE_COMMAND_WILDCARD) == "escape/fire/#");
    assert(std::string(EscapeTopic::FIRE_STATUS) == "escape/fire/status");
    assert(std::string(EscapeTopic::FIRE_FILM_ON) == "escape/fire/film-on");
    assert(std::string(EscapeTopic::FIRE_FILM_OFF) == "escape/fire/film-off");
    assert(std::string(EscapeTopic::FIRE_SOUND_LOOK) == "escape/fire/sound-look");
    assert(std::string(EscapeTopic::FIRE_SOUND_CRASH) == "escape/fire/sound-crash");
    assert(std::string(EscapeTopic::FIRE_SOUND_FAIL) == "escape/fire/sound-fail");
    assert(std::string(EscapeTopic::FIRE_SOUND_PASS) == "escape/fire/sound-pass");
    assert(std::string(EscapeTopic::FIRE_SOUND_BAKE) == "escape/fire/sound-bake");
    assert(std::string(EscapeTopic::FIRE_UNLOCK) == "escape/fire/unlock");
    assert(std::string(EscapeTopic::FIRE_RESET_ALL) == "escape/fire/reset-all");
    assert(std::string(EscapeTopic::FIRE_PANEL_LED_COMMAND) == "escape/cmd/fire-panel/led");

    return 0;
}
