$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "tests\build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$tests = @(
    @{
        Name = "controller_logic_test"
        Sources = @(
            "raspberry-pi-controller\test\controller_logic_test.cpp",
            "raspberry-pi-controller\src\GameController.cpp",
            "raspberry-pi-controller\src\puzzles\CopperPuzzle.cpp"
        )
        Includes = @("raspberry-pi-controller\src", "shared")
    },
    @{
        Name = "cubby_led_layout_test"
        Sources = @("escape-room-pico\test\test_cubby_led_layout.cpp")
        Includes = @("escape-room-pico\src")
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
        Name = "pico_post_mapping_test"
        Sources = @("tests\pico_post_mapping_test.cpp")
        Includes = @("shared")
    }
)

foreach ($test in $tests) {
    Write-Host "Building $($test.Name)..."

    $exe = Join-Path $buildDir "$($test.Name).exe"
    $sourceArgs = @($test.Sources | ForEach-Object { Join-Path $repoRoot $_ })
    $includeArgs = @()
    foreach ($include in $test.Includes) {
        $includeArgs += "-I"
        $includeArgs += (Join-Path $repoRoot $include)
    }

    & g++ -std=c++17 @includeArgs @sourceArgs -o $exe

    Write-Host "Running $($test.Name)..."
    & $exe
}

Write-Host "All host-side tests passed."
