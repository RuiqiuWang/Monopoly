$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$exe = Join-Path $root "movement_cli.exe"
$include = Join-Path $root "include"
$source = Join-Path $root "movement.c"
$cli_source = Join-Path $root "tests\movement_cli.c"

gcc -std=c11 -Wall -Wextra -Wpedantic -I $include `
    $source $cli_source -o $exe

$cases = @(
    @{ Input = "5"; Expected = "OK position=5" },
    @{ Input = "-1"; Expected = "ERROR invalid character: digits only" },
    @{ Input = "1.5"; Expected = "ERROR invalid character: digits only" },
    @{ Input = "125abd"; Expected = "ERROR invalid character: digits only" },
    @{ Input = "wada"; Expected = "ERROR invalid character: digits only" },
    @{ Input = "90|"; Expected = "ERROR invalid character: digits only" },
    @{ Input = "0"; Expected = "ERROR invalid step: must be greater than 0" },
    @{ Input = "69"; Expected = "OK position=69" },
    @{ Input = "70"; Expected = "OK position=0" },
    @{ Input = "2147483648"; Expected = "ERROR step out of range" }
)

$actual = $cases | ForEach-Object {
    $_.Input | & $exe
}

for ($i = 0; $i -lt $cases.Count; $i++) {
    if ($actual[$i] -ne $cases[$i].Expected) {
        throw "FAIL input '$($cases[$i].Input)': expected '$($cases[$i].Expected)', got '$($actual[$i])'"
    }
}

Write-Output "PASS: $($cases.Count) movement input cases"
