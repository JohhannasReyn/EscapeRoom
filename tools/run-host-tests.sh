#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/escape-room-host-tests.XXXXXX")"
run_id="$(date -u +%s)"

cleanup() {
    rm -rf "${build_dir}"
}
trap cleanup EXIT

if ! command -v g++ >/dev/null 2>&1; then
    echo "Missing g++. Install a C++ compiler first." >&2
    echo "Raspberry Pi: sudo apt install -y g++" >&2
    echo "macOS: xcode-select --install" >&2
    exit 1
fi

run_test() {
    local name="$1"
    shift

    local sources=()
    local includes=()
    while (($# > 0)); do
        case "$1" in
            --include)
                shift
                includes+=("-I" "${repo_root}/$1")
                ;;
            --source)
                shift
                sources+=("${repo_root}/$1")
                ;;
            *)
                echo "Unknown argument for ${name}: $1" >&2
                exit 1
                ;;
        esac
        shift
    done

    local exe="${build_dir}/${name}-${run_id}"
    echo "Building ${name}..."
    g++ -std=c++17 "${includes[@]}" "${sources[@]}" -o "${exe}"

    echo "Running ${name}..."
    "${exe}"
}

run_test "controller_logic_test" \
    --include "raspberry-pi-controller/src" \
    --include "shared" \
    --source "raspberry-pi-controller/test/controller_logic_test.cpp" \
    --source "raspberry-pi-controller/src/GameController.cpp" \
    --source "raspberry-pi-controller/src/puzzles/CopperPuzzle.cpp" \
    --source "raspberry-pi-controller/src/effects/DisplayOutput.cpp"

run_test "random_effect_test" \
    --include "raspberry-pi-controller/src" \
    --source "tests/random_effect_test.cpp" \
    --source "raspberry-pi-controller/src/effects/RandomEffect.cpp"

run_test "directory_audio_effect_test" \
    --include "raspberry-pi-controller/src" \
    --source "tests/directory_audio_effect_test.cpp" \
    --source "raspberry-pi-controller/src/effects/DirectoryAudioEffect.cpp" \
    --source "raspberry-pi-controller/src/effects/AudioEffect.cpp"

run_test "cubby_led_layout_test" \
    --include "pico1-cubby-approach-leds/src" \
    --source "pico1-cubby-approach-leds/test/test_cubby_led_layout.cpp"

run_test "post_state_test" \
    --include "shared" \
    --source "tests/post_state_test.cpp"

run_test "puzzle_debounce_test" \
    --include "shared" \
    --source "tests/puzzle_debounce_test.cpp"

run_test "pulse_timer_test" \
    --include "shared" \
    --source "tests/pulse_timer_test.cpp"

run_test "led_power_budget_test" \
    --include "shared" \
    --source "tests/led_power_budget_test.cpp"

run_test "component_diagnostics_test" \
    --include "pico-0-component-tests/src" \
    --source "tests/component_diagnostics_test.cpp"

run_test "oven_dial_test" \
    --include "shared" \
    --source "tests/oven_dial_test.cpp"

run_test "pico4_oven_range_test" \
    --source "tests/pico4_oven_range_test.cpp"

run_test "oven_thermometer_test" \
    --include "shared" \
    --source "tests/oven_thermometer_test.cpp"

run_test "protocol_topics_test" \
    --include "shared" \
    --source "tests/protocol_topics_test.cpp"

run_test "room_state_test" \
    --include "shared" \
    --source "tests/room_state_test.cpp"

run_test "pico_post_mapping_test" \
    --include "shared" \
    --source "tests/pico_post_mapping_test.cpp"

run_test "pico2_copper_single_input_test" \
    --source "tests/pico2_copper_single_input_test.cpp"

run_test "pico3_painting_always_active_test" \
    --source "tests/pico3_painting_always_active_test.cpp"

run_test "pico5_color_buttons_immediate_test" \
    --source "tests/pico5_color_buttons_immediate_test.cpp"

run_test "color_button_sequence_test" \
    --include "shared" \
    --source "tests/color_button_sequence_test.cpp"

run_test "pico_wifi_config_test" \
    --source "tests/pico_wifi_config_test.cpp"

run_test "sensor_always_active_test" \
    --source "tests/sensor_always_active_test.cpp"

run_test "pico_startup_status_test" \
    --source "tests/pico_startup_status_test.cpp"

run_test "room_wifi_defaults_test" \
    --source "tests/room_wifi_defaults_test.cpp"

run_test "rebase_flash_recommendation_test" \
    --source "tests/rebase_flash_recommendation_test.cpp"

run_test "fire_panel_tools_test" \
    --source "tests/fire_panel_tools_test.cpp"

run_test "audio_effect_low_latency_test" \
    --source "tests/audio_effect_low_latency_test.cpp"

run_test "room_cue_audio_test" \
    --source "tests/room_cue_audio_test.cpp"

echo "All host-side tests passed."
