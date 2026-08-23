$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Join-Path $root "movement_cli.exe"

gcc -std=c11 -Wall -Wextra -Wpedantic -I $root `
    (Join-Path $root "movement.c") (Join-Path $root "movement_cli.c") -o $exe

$cases = @(
    @{ Input = "5"; Expected = "OK position=5" },
    @{ Input = "-1"; Expected = "ERROR invalid input" },
    @{ Input = "1.5"; Expected = "ERROR invalid input" },
    @{ Input = "125abd"; Expected = "ERROR invalid input" },
    @{ Input = "wada"; Expected = "ERROR invalid input" },
    @{ Input = "90|"; Expected = "ERROR invalid input" },
    @{ Input = "0"; Expected = "ERROR invalid input" },
    @{ Input = "69"; Expected = "OK position=69" },
    @{ Input = "70"; Expected = "OK position=0" },
    @{ Input = "2147483648"; Expected = "ERROR invalid input" }
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
