#include <cassert>
#include <string>

#include "../shared/RoomState.h"

int main() {
    assert(std::string(roomStateName(RoomState::WAITING_FOR_CUBBY_APPROACH)) == "WAITING_FOR_CUBBY_APPROACH");
    assert(std::string(roomStateName(RoomState::CUBBY_APPROACH_DETECTED)) == "CUBBY_APPROACH_DETECTED");
    assert(std::string(roomStateName(RoomState::FIRST_CUBBY_LIT)) == "FIRST_CUBBY_LIT");
    assert(std::string(roomStateName(RoomState::COPPER_PUZZLE_ACTIVE)) == "COPPER_PUZZLE_ACTIVE");
    assert(std::string(roomStateName(RoomState::COPPER_PUZZLE_COMPLETE)) == "COPPER_PUZZLE_COMPLETE");
    assert(std::string(roomStateName(RoomState::BOTTLE_LOCK_STAGE)) == "BOTTLE_LOCK_STAGE");
    assert(std::string(roomStateName(RoomState::PADLOCK_BOX_STAGE)) == "PADLOCK_BOX_STAGE");
    assert(std::string(roomStateName(RoomState::RFID_STAGE)) == "RFID_STAGE");
    assert(std::string(roomStateName(RoomState::PAINTING_ROTATION_ACTIVE)) == "PAINTING_ROTATION_ACTIVE");
    assert(std::string(roomStateName(RoomState::PAINTING_ROTATION_COMPLETE)) == "PAINTING_ROTATION_COMPLETE");
    assert(std::string(roomStateName(RoomState::CRASHING_PLATES_PLAYED)) == "CRASHING_PLATES_PLAYED");
    assert(std::string(roomStateName(RoomState::FINAL_PIECE_ACTIVE)) == "FINAL_PIECE_ACTIVE");
    assert(std::string(roomStateName(RoomState::FINAL_PIECE_PLACED)) == "FINAL_PIECE_PLACED");
    assert(std::string(roomStateName(RoomState::SMART_FILM_REVEALED)) == "SMART_FILM_REVEALED");
    assert(std::string(roomStateName(RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE)) == "COLOR_BUTTON_SEQUENCE_ACTIVE");
    assert(std::string(roomStateName(RoomState::COLOR_BUTTON_SEQUENCE_COMPLETE)) == "COLOR_BUTTON_SEQUENCE_COMPLETE");
    assert(std::string(roomStateName(RoomState::DISPLAY_BAKE_350)) == "DISPLAY_BAKE_350");
    assert(std::string(roomStateName(RoomState::OVEN_KNOB_ACTIVE)) == "OVEN_KNOB_ACTIVE");
    assert(std::string(roomStateName(RoomState::OVEN_TARGET_REACHED)) == "OVEN_TARGET_REACHED");
    assert(std::string(roomStateName(RoomState::ELECTROMAGNETIC_LOCK_RELEASED)) == "ELECTROMAGNETIC_LOCK_RELEASED");
    assert(std::string(roomStateName(RoomState::ROOM_KEY_AVAILABLE)) == "ROOM_KEY_AVAILABLE");

    return 0;
}
