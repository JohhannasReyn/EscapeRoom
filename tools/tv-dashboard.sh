#!/usr/bin/env bash
set -euo pipefail

MQTT_HOST="${MQTT_HOST:-localhost}"

declare -A puzzle_state=(
    [1]="pending"
    [2]="pending"
    [3]="pending"
    [4]="pending"
    [5]="pending"
)

declare -A post_state=(
    [1]="unknown"
    [2]="unknown"
    [3]="unknown"
    [4]="unknown"
    [5]="unknown"
)

last_event="Waiting for puzzle events..."
bake_message=0
room_complete=0
oven_value="--"

mark_done() {
    puzzle_state["$1"]="done"
}

clear_done() {
    puzzle_state["$1"]="pending"
}

progress_percent() {
    local done=0
    local index

    for index in 1 2 3 4 5; do
        if [ "${puzzle_state[$index]}" = "done" ]; then
            done=$((done + 1))
        fi
    done

    echo $((done * 100 / 5))
}

box_for() {
    local value="${puzzle_state[$1]}"

    if [ "${value}" = "done" ]; then
        echo "[x]"
    else
        echo "[ ]"
    fi
}

render() {
    clear || true
    echo "Escape Room Status"
    echo "=================="
    echo

    if [ "${room_complete}" -eq 1 ]; then
        echo "Well Done!"
        echo
    elif [ "${bake_message}" -eq 1 ]; then
        echo "Bake at 350 Degrees"
        echo
        echo "Oven set to: ${oven_value} Degrees"
        echo
    else
        echo "Progress: $(progress_percent)%"
        echo
    fi

    echo "$(box_for 1) Pico 1  Cubbies / approach       POST: ${post_state[1]}"
    echo "$(box_for 2) Pico 2  Copper / final piece     POST: ${post_state[2]}"
    echo "$(box_for 3) Pico 3  Painting rotation        POST: ${post_state[3]}"
    echo "$(box_for 4) Pico 4  Smart film / oven        POST: ${post_state[4]}"
    echo "$(box_for 5) Pico 5  Color buttons            POST: ${post_state[5]}"
    echo
    echo "Last event:"
    echo "${last_event}"
    echo
    echo "This dashboard listens to MQTT only. It is safe to leave running."
}

handle_message() {
    local topic="$1"
    local payload="$2"

    case "${topic}" in
        escape/post/cubby/1/state) post_state[1]="${payload}"; [ "${payload}" = "ready" ] && clear_done 1 ;;
        escape/post/cubby/2/state) post_state[2]="${payload}"; [ "${payload}" = "ready" ] && clear_done 2 ;;
        escape/post/cubby/3/state) post_state[3]="${payload}"; [ "${payload}" = "ready" ] && clear_done 3 ;;
        escape/post/cubby/4/state) post_state[4]="${payload}"; [ "${payload}" = "ready" ] && clear_done 4 ;;
        escape/post/cubby/5/state) post_state[5]="${payload}"; [ "${payload}" = "ready" ] && clear_done 5 ;;
        escape/pico1/cubby_approach_detected) mark_done 1 ;;
        escape/pico2/copper_puzzle_complete|escape/pico2/final_piece_placed) mark_done 2 ;;
        escape/pico3/painting_rotation_complete) mark_done 3 ;;
        escape/pico4/oven_target_reached) mark_done 4 ;;
        escape/pico4/oven_position_update|escape/oven/degrees)
            oven_value="${payload}"
            ;;
        escape/telemetry/pico4/oven)
            if [[ "${payload}" =~ oven_value=([0-9]+) ]]; then
                oven_value="${BASH_REMATCH[1]}"
            fi
            ;;
        escape/pico4/electromag_lock_unlocked)
            mark_done 4
            room_complete=1
            bake_message=0
            ;;
        escape/pico5/color_sequence_complete)
            mark_done 5
            bake_message=1
            ;;
        escape/game/reset|escape/cmd/all/reset_puzzle)
            for index in 1 2 3 4 5; do
                clear_done "${index}"
                post_state["${index}"]="unknown"
            done
            bake_message=0
            room_complete=0
            oven_value="--"
            ;;
    esac

    if [[ "${topic}" != escape/telemetry/* ]]; then
        last_event="${topic} ${payload}"
    fi
}

render

mosquitto_sub -h "${MQTT_HOST}" -v -t 'escape/#' | while read -r topic payload; do
    handle_message "${topic}" "${payload:-}"
    render
done
