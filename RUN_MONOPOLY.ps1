$ErrorActionPreference = "Stop"
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
chcp 65001 > $null

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$cc = $env:CC
if ([string]::IsNullOrWhiteSpace($cc)) {
    $cc = "gcc"
}
$python = $env:PYTHON
if ([string]::IsNullOrWhiteSpace($python)) {
    $python = "python"
}
$cflags = $env:CFLAGS
if ([string]::IsNullOrWhiteSpace($cflags)) {
    $cflags = "-std=c11 -Wall -Wextra -Wpedantic"
}
$cppflags = $env:CPPFLAGS
if ([string]::IsNullOrWhiteSpace($cppflags)) {
    $cppflags = "-Iinclude"
}

function Write-Step {
    param(
        [int]$Index,
        [int]$Total,
        [string]$Label,
        [string]$Verb = "Running"
    )
    Write-Host "[$Index/$Total] $Verb $Label ... " -NoNewline
}

function Write-Pass {
    Write-Host "[PASS]"
}

function Write-Fail {
    param([string]$Message)
    Write-Host "[FAIL]"
    if ($Message) {
        Write-Host $Message
    }
    exit 1
}

function Invoke-Checked {
    param(
        [int]$Index,
        [int]$Total,
        [string]$Label,
        [scriptblock]$Command,
        [string]$Verb = "Running"
    )

    Write-Step -Index $Index -Total $Total -Label $Label -Verb $Verb
    try {
        & $Command
        Write-Pass
    } catch {
        Write-Fail $_.Exception.Message
    }
}

function Assert-Exists {
    param(
        [int]$Index,
        [int]$Total,
        [string]$Path,
        [string]$Label
    )

    Write-Step -Index $Index -Total $Total -Label $Label -Verb "Checking"
    if (Test-Path $Path) {
        Write-Pass
    } else {
        Write-Fail "Missing $Path"
    }
}

function Resolve-Executable {
    param([string]$BasePath)
    if (Test-Path $BasePath) {
        return $BasePath
    }
    if (Test-Path ($BasePath + ".exe")) {
        return ($BasePath + ".exe")
    }
    return $BasePath
}

function Invoke-Compiler {
    param([string[]]$Arguments)

    Push-Location $root
    try {
        & $cc @Arguments | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "compiler failed"
        }
    } finally {
        Pop-Location
    }
}

function Remove-IfExists {
    param([string[]]$Paths)

    foreach ($path in $Paths) {
        $fullPath = Join-Path $root $path
        if (Test-Path $fullPath) {
            Remove-Item -Force $fullPath
        }
    }
}

function Build-Target {
    param([string]$Target)

    $makeCommand = Get-Command make -ErrorAction SilentlyContinue
    if ($makeCommand) {
        & make -C $root $Target | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "build failed"
        }
        return
    }

    $commonFlags = @()
    $commonFlags += $cppflags.Split(" ", [System.StringSplitOptions]::RemoveEmptyEntries)
    $commonFlags += $cflags.Split(" ", [System.StringSplitOptions]::RemoveEmptyEntries)

    switch ($Target) {
        "clean" {
            Remove-IfExists @(
                "test_movement",
                "test_movement.exe",
                "movement_cli",
                "movement_cli.exe",
                "character_select_cli",
                "character_select_cli.exe",
                "game_engine",
                "game_engine.exe",
                "tutorial_test",
                "tutorial_test.exe",
                "command_test",
                "command_test.exe",
                "mine_test",
                "mine_test.exe",
                "tool_room_test",
                "tool_room_test.exe",
                "assets_test",
                "assets_test.exe",
                "character_select_test",
                "character_select_test.exe",
                "map_test",
                "map_test.exe",
                "item_usage_test.exe",
                "item_effect_test.exe",
                "fortune_test.exe",
                "gift_house_test.exe",
                "help_query_test.exe",
                "property_test.exe"
            )
        }
        "test_movement" {
            Invoke-Compiler ($commonFlags + @("movement.c", "tests\test_movement.c", "-o", "test_movement.exe"))
        }
        "movement_cli" {
            Invoke-Compiler ($commonFlags + @("movement.c", "tests\movement_cli.c", "-o", "movement_cli.exe"))
        }
        "character_select_cli" {
            Invoke-Compiler ($commonFlags + @("character_select.c", "input.c", "character_select_cli.c", "-o", "character_select_cli.exe"))
        }
        "game_engine" {
            Invoke-Compiler ($commonFlags + @("game_engine.c", "command.c", "tutorial.c", "map.c", "tui.c", "movement.c", "mine.c", "tool_room.c", "assets.c", "player.c", "character_select.c", "input.c", "item_usage.c", "item_effect.c", "fortune.c", "gift_house.c", "help_query.c", "property.c", "-o", "game_engine.exe"))
        }
        "tutorial_test" {
            Invoke-Compiler ($commonFlags + @("tutorial.c", "input.c", "tui.c", "map.c", "movement.c", "tests\test_tutorial.c", "-o", "tutorial_test.exe"))
        }
        "command_test" {
            Invoke-Compiler ($commonFlags + @("command.c", "tests\test_command.c", "-o", "command_test.exe"))
        }
        "mine_test" {
            Invoke-Compiler ($commonFlags + @("mine.c", "map.c", "player.c", "tests\test_mine.c", "-o", "mine_test.exe"))
        }
        "tool_room_test" {
            Invoke-Compiler ($commonFlags + @("tool_room.c", "input.c", "player.c", "tests\test_tool_room.c", "-o", "tool_room_test.exe"))
        }
        "assets_test" {
            Invoke-Compiler ($commonFlags + @("assets.c", "map.c", "player.c", "tests\test_assets.c", "-o", "assets_test.exe"))
        }
        "character_select_test" {
            Invoke-Compiler ($commonFlags + @("character_select.c", "input.c", "tests\test_character_select.c", "-o", "character_select_test.exe"))
        }
        "map_test" {
            Invoke-Compiler ($commonFlags + @("map.c", "tests\test_map.c", "-o", "map_test.exe"))
        }
        "item_usage_test" {
            Invoke-Compiler ($commonFlags + @("item_usage.c", "map.c", "tests\test_item_usage.c", "-o", "item_usage_test.exe"))
        }
        "item_effect_test" {
            Invoke-Compiler ($commonFlags + @("item_effect.c", "movement.c", "map.c", "tests\test_item_effect.c", "-o", "item_effect_test.exe"))
        }
        "fortune_test" {
            Invoke-Compiler ($commonFlags + @("fortune.c", "property.c", "map.c", "player.c", "tests\test_fortune.c", "-o", "fortune_test.exe"))
        }
        "gift_house_test" {
            Invoke-Compiler ($commonFlags + @("gift_house.c", "input.c", "tests\test_gift_house.c", "-o", "gift_house_test.exe"))
        }
        "help_query_test" {
            Invoke-Compiler ($commonFlags + @("help_query.c", "tests\test_help_query.c", "-o", "help_query_test.exe"))
        }
        "property_test" {
            Invoke-Compiler ($commonFlags + @("property.c", "map.c", "tests\test_property.c", "-o", "property_test.exe"))
        }
        default {
            throw "No fallback build rule for $Target"
        }
    }
}

Write-Host "MONOPOLY automated environment/build/test runner"
Write-Host "Project: $root"
Write-Host "Mode: dependency check -> build -> test"
Write-Host ""

Assert-Exists 1 4 (Join-Path $root "Makefile") "project layout"

$envChecks = @(
    @{ Name = $cc; Label = "C compiler" },
    @{ Name = $python; Label = "python" }
)
for ($i = 0; $i -lt $envChecks.Count; $i++) {
    Write-Step -Index ($i + 2) -Total 4 -Label $envChecks[$i].Label -Verb "Checking"
    if (Get-Command $envChecks[$i].Name -ErrorAction SilentlyContinue) {
        Write-Pass
    } else {
        Write-Fail "Missing $($envChecks[$i].Label)"
    }
}
Write-Step -Index 4 -Total 4 -Label "make" -Verb "Checking"
if (Get-Command make -ErrorAction SilentlyContinue) {
    Write-Pass
} else {
    Write-Host "[SKIP]"
    Write-Host "make not found; direct compiler fallback will be used."
}

Write-Host ""

$buildSteps = @(
    "clean",
    "test_movement",
    "movement_cli",
    "character_select_cli",
    "tutorial_test",
    "command_test",
    "mine_test",
    "tool_room_test",
    "assets_test",
    "character_select_test",
    "map_test",
    "item_usage_test",
    "item_effect_test",
    "fortune_test",
    "gift_house_test",
    "help_query_test",
    "property_test",
    "game_engine"
)
for ($i = 0; $i -lt $buildSteps.Count; $i++) {
    $target = $buildSteps[$i]
    $driver = if (Get-Command make -ErrorAction SilentlyContinue) { "make" } else { "$cc fallback" }
    Write-Step -Index ($i + 1) -Total $buildSteps.Count -Label "$target using $driver" -Verb "Building"
    try {
        Build-Target $target
        Write-Pass
    } catch {
        Write-Fail $_.Exception.Message
    }
}

Write-Host ""

$unitTests = @(
    @{ Name = "movement unit"; Path = "test_movement" },
    @{ Name = "tutorial choice unit"; Path = "tutorial_test" },
    @{ Name = "command unit"; Path = "command_test" },
    @{ Name = "mine unit"; Path = "mine_test" },
    @{ Name = "tool room unit"; Path = "tool_room_test" },
    @{ Name = "assets unit"; Path = "assets_test" },
    @{ Name = "character select unit"; Path = "character_select_test" },
    @{ Name = "map property unit"; Path = "map_test" },
    @{ Name = "item usage unit"; Path = "item_usage_test" },
    @{ Name = "item effect unit"; Path = "item_effect_test" },
    @{ Name = "fortune unit"; Path = "fortune_test" },
    @{ Name = "gift house unit"; Path = "gift_house_test" },
    @{ Name = "help query unit"; Path = "help_query_test" },
    @{ Name = "property unit"; Path = "property_test" }
)
$testTotal = $unitTests.Count + 2
for ($i = 0; $i -lt $unitTests.Count; $i++) {
    $unitExe = Resolve-Executable (Join-Path $root $unitTests[$i].Path)
    Write-Step -Index ($i + 1) -Total $testTotal -Label $unitTests[$i].Name -Verb "Running Test"
    try {
        & $unitExe | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "$($unitTests[$i].Name) failed" }
        Write-Pass
    } catch { Write-Fail $_.Exception.Message }
}
$testIndex = $unitTests.Count + 1
Write-Step -Index $testIndex -Total $testTotal -Label "JSON schema 2.0 suite" -Verb "Running Test"
try {
    & $python (Join-Path $root "tests\validate_json_testcases.py") | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "JSON schema validation failed" }
    & $python (Join-Path $root "tests\run_json_tests.py") | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "JSON schema 2.0 suite failed" }
    Write-Pass
} catch {
    Write-Fail $_.Exception.Message
}
$testIndex++

Write-Step -Index $testIndex -Total $testTotal -Label "game engine end-to-end" -Verb "Running Test"
try {
    & $python (Join-Path $root "tests\e2e_game.py")
    if ($LASTEXITCODE -ne 0) { throw "game engine end-to-end failed" }
    Write-Pass
} catch { Write-Fail $_.Exception.Message }

Write-Host ""
Write-Host "All checks passed: $($buildSteps.Count) build steps, $testTotal tests."
