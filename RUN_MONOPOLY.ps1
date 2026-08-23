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

function Run-JsonCase {
    param(
        [int]$Index,
        [int]$Total,
        [string]$CaseFile
    )

    $relative = $CaseFile.Substring(($root + "\tests\input\").Length)
    $outputFile = Join-Path $root ("tests\output\" + $relative)
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
    @{ Name = "make"; Label = "make" },
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
    Write-Step -Index ($i + 1) -Total $buildSteps.Count -Label $target -Verb "Building"
    try {
        & make -C $root $target | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "build failed"
        }
        Write-Pass
    } catch {
        Write-Fail $_.Exception.Message
    }
}

Write-Host ""

$cases = @()
$cases += Join-Path $root "tests\input\TC-CTRL-001.json"
$cases += Join-Path $root "tests\input\TC-DISPLAY-001.json"
$cases += Join-Path $root "tests\input\TC-ROLL-001.json"
$cases += Join-Path $root "tests\input\TC-STEP-001.json"
$cases += Join-Path $root "tests\input\TC-STEP-002.json"
$cases += Join-Path $root "tests\input\TC-TURN-001.json"
$cases += Get-ChildItem (Join-Path $root "tests\input\from_test") -Recurse -Filter *.json | ForEach-Object { $_.FullName } | Sort-Object

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
