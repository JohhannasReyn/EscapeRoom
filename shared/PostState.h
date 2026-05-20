#ifndef ESCAPE_ROOM_POST_STATE_H
#define ESCAPE_ROOM_POST_STATE_H

#include <string>

inline const char* postStatePayload(bool completed) {
    return completed ? "completed" : "ready";
}

inline std::string postStateTopic(int cubbyNumber) {
    return "escape/post/cubby/" + std::to_string(cubbyNumber) + "/state";
}

inline std::string cubbyStatusTopic(int cubbyNumber) {
    return "escape/cubby/" + std::to_string(cubbyNumber) + "/status";
}

#endif
