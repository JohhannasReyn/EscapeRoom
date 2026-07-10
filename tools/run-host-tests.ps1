$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "tests\build"
$runId = [DateTime]::UtcNow.Ticks
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$tests = @(
    @{
        Name = "controller_logic_test"
        Sources = @(
            "raspberry-pi-controller\test\controller_logic_test.cpp",
            "raspberry-pi-controller\src\GameController.cpp",
            "raspberry-pi-controller\src\puzzles\CopperPuzzle.cpp",
            "raspberry-pi-controller\src\effects\DisplayOutput.cpp"
        )
        Includes = @("raspberry-pi-controller\src", "shared")
    },
    @{
        Name = "random_effect_test"
        Sources = @(
            "tests\random_effect_test.cpp",
            "raspberry-pi-controller\src\effects\RandomEffect.cpp"
        )
        Includes = @("raspberry-pi-controller\src")
    },
    @{
        Name = "cubby_led_layout_test"
        Sources = @("pico1-cubby-approach-leds\test\test_cubby_led_layout.cpp")
        Includes = @("pico1-cubby-approach-leds\src")
    },
    @{
        Name = "post_state_test"
        Sources = @("tests\post_state_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "puzzle_debounce_test"
        Sources = @("tests\puzzle_debounce_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "pulse_timer_test"
        Sources = @("tests\pulse_timer_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "led_power_budget_test"
        Sources = @("tests\led_power_budget_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "component_diagnostics_test"
        Sources = @("tests\component_diagnostics_test.cpp")
        Includes = @("pico-0-component-tests\src")
    },
    @{
        Name = "oven_dial_test"
        Sources = @("tests\oven_dial_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "pico4_oven_range_test"
        Sources = @("tests\pico4_oven_range_test.cpp")
        Includes = @()
    },
    @{
        Name = "oven_thermometer_test"
        Sources = @("tests\oven_thermometer_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "protocol_topics_test"
        Sources = @("tests\protocol_topics_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "room_state_test"
        Sources = @("tests\room_state_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "pico_post_mapping_test"
        Sources = @("tests\pico_post_mapping_test.cpp")
        Includes = @("shared")
    },
    @{
        Name = "pico2_copper_single_input_test"
        Sources = @("tests\pico2_copper_single_input_test.cpp")
        Includes = @()
    },
    @{
        Name = "pico3_painting_always_active_test"
        Sources = @("tests\pico3_painting_always_active_test.cpp")
        Includes = @()
    },
    @{
        Name = "pico5_color_buttons_immediate_test"
        Sources = @("tests\pico5_color_buttons_immediate_test.cpp")
        Includes = @()
    },
    @{
        Name = "pico_wifi_config_test"
        Sources = @("tests\pico_wifi_config_test.cpp")
        Includes = @()
    },
    @{
        Name = "sensor_always_active_test"
        Sources = @("tests\sensor_always_active_test.cpp")
        Includes = @()
    },
    @{
        Name = "pico_startup_status_test"
        Sources = @("tests\pico_startup_status_test.cpp")
        Includes = @()
    },
    @{
        Name = "room_wifi_defaults_test"
        Sources = @("tests\room_wifi_defaults_test.cpp")
        Includes = @()
    },
    @{
        Name = "fire_panel_tools_test"
        Sources = @("tests\fire_panel_tools_test.cpp")
        Includes = @()
    },
    @{
        Name = "audio_effect_low_latency_test"
        Sources = @("tests\audio_effect_low_latency_test.cpp")
        Includes = @()
    },
    @{
        Name = "room_cue_audio_test"
        Sources = @("tests\room_cue_audio_test.cpp")
        Includes = @()
    }
)

foreach ($test in $tests) {
    Write-Host "Building $($test.Name)..."

    $exe = Join-Path $buildDir "$($test.Name)-$runId.exe"
    $sourceArgs = @($test.Sources | ForEach-Object { Join-Path $repoRoot $_ })
    $includeArgs = @()
    foreach ($include in $test.Includes) {
        $includeArgs += "-I"
        $includeArgs += (Join-Path $repoRoot $include)
    }

    & g++ -std=c++17 @includeArgs @sourceArgs -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $($test.Name)."
    }

    $ran = $false
    for ($attempt = 1; $attempt -le 2 -and -not $ran; $attempt++) {
        if ($attempt -gt 1) {
            $exe = Join-Path $buildDir "$($test.Name)-$runId-retry$attempt.exe"
            & g++ -std=c++17 @includeArgs @sourceArgs -o $exe
            if ($LASTEXITCODE -ne 0) {
                throw "Build failed for $($test.Name) retry $attempt."
            }
        }

        Write-Host "Running $($test.Name)..."
        try {
            & $exe
            if ($LASTEXITCODE -ne 0) {
                throw "Test failed: $($test.Name)."
            }
            $ran = $true
        } catch {
            if ($attempt -ge 2) {
                throw
            }

            Write-Host "Retrying $($test.Name) after local execution policy blocked the first executable..."
        }
    }
}

Write-Host "All host-side tests passed."
