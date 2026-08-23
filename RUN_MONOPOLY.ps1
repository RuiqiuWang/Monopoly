$ErrorActionPreference = "Stop"

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

function Get-CaseId {
    param([string]$CaseFile)
    & $python -c "import json,sys;from pathlib import Path;case_file=Path(sys.argv[1]);print(json.loads(case_file.read_text(encoding='utf-8')).get('case_id', case_file.stem) if case_file.exists() else case_file.stem)" $CaseFile
    if ($LASTEXITCODE -ne 0) {
        return [IO.Path]::GetFileNameWithoutExtension($CaseFile)
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
                "game_engine",
                "game_engine.exe",
                "tests\json_runner",
                "tests\json_runner.exe"
            )
        }
        "test_movement" {
            Invoke-Compiler ($commonFlags + @("movement.c", "tests\test_movement.c", "-o", "test_movement.exe"))
        }
        "movement_cli" {
            Invoke-Compiler ($commonFlags + @("movement.c", "tests\movement_cli.c", "-o", "movement_cli.exe"))
        }
        "game_engine" {
            Invoke-Compiler ($commonFlags + @("game_engine.c", "map.c", "tui.c", "movement.c", "-o", "game_engine.exe"))
        }
        "tests/json_runner" {
            Invoke-Compiler ($commonFlags + @("movement.c", "tests\json_engine.c", "tests\json_runner.c", "-o", "tests\json_runner.exe"))
        }
        default {
            throw "No fallback build rule for $Target"
        }
    }
}

function Run-JsonCase {
    param(
        [int]$Index,
        [int]$Total,
        [string]$CaseFile
    )

    $outputFile = Join-Path $root ("tests\output\" + [IO.Path]::GetFileName($CaseFile))
    $outputDir = Split-Path -Parent $outputFile
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir | Out-Null
    }
    $caseId = Get-CaseId -CaseFile $CaseFile
    Write-Step -Index $Index -Total $Total -Label $caseId -Verb "Running Test"

    try {
        $runnerScript = @'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
case_file = Path(sys.argv[2])
output_file = Path(sys.argv[3])
sys.path.insert(0, str(root / "tests"))

from run_json_tests import run_case_file

report = run_case_file(case_file, output_file)
if report.get("result") != "PASS":
    print(json.dumps(report.get("errors", []), ensure_ascii=False))
    raise SystemExit(1)
'@
        & $python -c $runnerScript $root $CaseFile $outputFile
        if ($LASTEXITCODE -ne 0) {
            throw "json test failed"
        }
        Write-Pass
    } catch {
        Write-Fail $_.Exception.Message
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
    "game_engine",
    "tests/json_runner"
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

$cases = Get-ChildItem (Join-Path $root "tests\input") -Filter *.json | Sort-Object Name | ForEach-Object { $_.FullName }

$movementExe = Resolve-Executable (Join-Path $root "test_movement")
Write-Step -Index 1 -Total 1 -Label "movement unit" -Verb "Running Test"
try {
    & $movementExe | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "movement unit failed"
    }
    Write-Pass
} catch {
    Write-Fail $_.Exception.Message
}

$testIndex = 2
$testTotal = 1 + $cases.Count
foreach ($case in $cases) {
    Run-JsonCase -Index $testIndex -Total $testTotal -CaseFile $case
    $testIndex++
}

Write-Host ""
Write-Host "All checks passed: $($buildSteps.Count) build steps, $testTotal tests."
