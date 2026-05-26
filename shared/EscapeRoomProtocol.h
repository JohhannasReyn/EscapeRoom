#ifndef ESCAPE_ROOM_PROTOCOL_H
#define ESCAPE_ROOM_PROTOCOL_H

namespace EscapeTopic {
inline constexpr const char* CUBBY_APPROACH_DETECTED = "escape/pico1/cubby_approach_detected";
inline constexpr const char* CUBBY_LED_READY = "escape/pico1/cubby_led_ready";
inline constexpr const char* CUBBY_LED_ERROR = "escape/pico1/cubby_led_error";

inline constexpr const char* COPPER_PUZZLE_COMPLETE = "escape/pico2/copper_puzzle_complete";
inline constexpr const char* FINAL_PIECE_PLACED = "escape/pico2/final_piece_placed";

inline constexpr const char* PAINTING_ROTATION_COMPLETE = "escape/pico3/painting_rotation_complete";

inline constexpr const char* SMART_FILM_READY = "escape/pico4/smart_film_ready";
inline constexpr const char* OVEN_HOME_DETECTED = "escape/pico4/oven_home_detected";
inline constexpr const char* OVEN_POSITION_UPDATE = "escape/pico4/oven_position_update";
inline constexpr const char* OVEN_TARGET_REACHED = "escape/pico4/oven_target_reached";
inline constexpr const char* ELECTROMAG_LOCK_UNLOCKED = "escape/pico4/electromag_lock_unlocked";
inline constexpr const char* OVEN_ERROR = "escape/pico4/oven_error";

inline constexpr const char* COLOR_SEQUENCE_COMPLETE = "escape/pico5/color_sequence_complete";
inline constexpr const char* COLOR_SEQUENCE_ERROR = "escape/pico5/color_sequence_error";

inline constexpr const char* ENABLE_CUBBY_LIGHT = "escape/cmd/pico1/enable_cubby_light";
inline constexpr const char* ENABLE_COPPER_PUZZLE = "escape/cmd/pico2/enable_copper_puzzle";
inline constexpr const char* ENABLE_PAINTING_ROTATION = "escape/cmd/pico3/enable_painting_rotation";
inline constexpr const char* REVEAL_SMART_FILM = "escape/cmd/pico4/reveal_smart_film";
inline constexpr const char* ENABLE_COLOR_BUTTON_SEQUENCE = "escape/cmd/pico5/enable_color_button_sequence";
inline constexpr const char* ENABLE_OVEN_KNOB = "escape/cmd/pico4/enable_oven_knob";
inline constexpr const char* UNLOCK_ELECTROMAG_LOCK = "escape/cmd/pico4/unlock_electromag_lock";
inline constexpr const char* RESET_PUZZLE = "escape/cmd/all/reset_puzzle";
inline constexpr const char* STATUS_REQUEST = "escape/cmd/all/status_request";

inline constexpr const char* LEGACY_GAME_RESET = "escape/game/reset";
inline constexpr const char* LEGACY_POST_QUERY = "escape/post/query";
inline constexpr const char* LEGACY_CUBBY_1_LIGHT_ON = "escape/cubby/1/light_on";
inline constexpr const char* LEGACY_OVEN_ENABLE = "escape/oven/enable";
inline constexpr const char* LEGACY_OVEN_DEGREES = "escape/oven/degrees";
inline constexpr const char* LEGACY_PDLC_ON = "escape/pdlc/on";
inline constexpr const char* LEGACY_LOCK_TRIGGER = "escape/lock/trigger";
} // namespace EscapeTopic

#endif
