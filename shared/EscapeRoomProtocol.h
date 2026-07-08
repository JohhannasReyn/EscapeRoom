#ifndef ESCAPE_ROOM_PROTOCOL_H
#define ESCAPE_ROOM_PROTOCOL_H

namespace EscapeTopic {
inline constexpr const char* CUBBY_APPROACH_DETECTED = "escape/pico1/cubby_approach_detected";
inline constexpr const char* CUBBY_LED_READY = "escape/pico1/cubby_led_ready";
inline constexpr const char* CUBBY_LED_ERROR = "escape/pico1/cubby_led_error";

inline constexpr const char* COPPER_PUZZLE_COMPLETE = "escape/pico2/copper_puzzle_complete";

inline constexpr const char* PAINTING_ROTATION_COMPLETE = "escape/pico3/painting_rotation_complete";

inline constexpr const char* SMART_FILM_READY = "escape/pico4/smart_film_ready";
inline constexpr const char* OVEN_POSITION_UPDATE = "escape/pico4/oven_position_update";
inline constexpr const char* OVEN_TARGET_REACHED = "escape/pico4/oven_target_reached";
inline constexpr const char* ELECTROMAG_LOCK_UNLOCKED = "escape/pico4/electromag_lock_unlocked";
inline constexpr const char* OVEN_ERROR = "escape/pico4/oven_error";

inline constexpr const char* COLOR_SEQUENCE_COMPLETE = "escape/pico5/color_sequence_complete";
inline constexpr const char* COLOR_SEQUENCE_ERROR = "escape/pico5/color_sequence_error";

inline constexpr const char* ENABLE_CUBBY_LIGHT = "escape/cmd/pico1/enable_cubby_light";
inline constexpr const char* REVEAL_SMART_FILM = "escape/cmd/pico4/reveal_smart_film";
inline constexpr const char* UNLOCK_ELECTROMAG_LOCK = "escape/cmd/pico4/unlock_electromag_lock";
inline constexpr const char* RESET_PUZZLE = "escape/cmd/all/reset_puzzle";
inline constexpr const char* STATUS_REQUEST = "escape/cmd/all/status_request";
inline constexpr const char* PICO_STATUS_REPORT = "escape/status/pico";
inline constexpr const char* SENSOR_TELEMETRY_WILDCARD = "escape/telemetry/#";
inline constexpr const char* FIRE_COMMAND_WILDCARD = "escape/fire/#";
inline constexpr const char* FIRE_STATUS = "escape/fire/status";
inline constexpr const char* FIRE_FILM_ON = "escape/fire/film-on";
inline constexpr const char* FIRE_FILM_OFF = "escape/fire/film-off";
inline constexpr const char* FIRE_SOUND_LOOK = "escape/fire/sound-look";
inline constexpr const char* FIRE_SOUND_CRASH = "escape/fire/sound-crash";
inline constexpr const char* FIRE_SOUND_FAIL = "escape/fire/sound-fail";
inline constexpr const char* FIRE_SOUND_PASS = "escape/fire/sound-pass";
inline constexpr const char* FIRE_SOUND_BAKE = "escape/fire/sound-bake";
inline constexpr const char* FIRE_UNLOCK = "escape/fire/unlock";
inline constexpr const char* FIRE_RESET_ALL = "escape/fire/reset-all";
inline constexpr const char* FIRE_PANEL_LED_COMMAND = "escape/cmd/fire-panel/led";

inline constexpr const char* LEGACY_GAME_RESET = "escape/game/reset";
inline constexpr const char* LEGACY_POST_QUERY = "escape/post/query";
inline constexpr const char* LEGACY_CUBBY_1_LIGHT_ON = "escape/cubby/1/light_on";
inline constexpr const char* LEGACY_OVEN_DEGREES = "escape/oven/degrees";
inline constexpr const char* LEGACY_PDLC_ON = "escape/pdlc/on";
inline constexpr const char* LEGACY_LOCK_TRIGGER = "escape/lock/trigger";
} // namespace EscapeTopic

#endif
