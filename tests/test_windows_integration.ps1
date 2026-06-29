# Windows integration tests for Brick
# Tests: brick build, brick run, brick --version

$ErrorActionPreference = "Stop"
$rootDir = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$brick = Join-Path $rootDir "build\brick.exe"
$passed = 0
$failed = 0

function Test-Command($name, $script) {
    try {
        & $script
        Write-Host "  [PASS] $name"
        $script:passed++
    } catch {
        Write-Host "  [FAIL] $name : $($_.Exception.Message)"
        $script:failed++
    }
}

function Exec-Check($exe, $argsArr) {
    # Use cmd /c to reliably capture output and exit code
    $argStr = ($argsArr | ForEach-Object { if ($_ -match '\s') { """$_""" } else { "$_" } }) -join ' '
    $combined = & cmd /c "`"$exe`" $argStr 2>&1"
    $ec = $LASTEXITCODE
    if ($ec -ne 0) {
        $text = $combined -join "`n"
        throw "exit code $ec`n$text"
    }
    return ($combined -join "`n")
}

function Exec-Check-Should-Fail($exe, $argsArr) {
    $argStr = ($argsArr | ForEach-Object { if ($_ -match '\s') { """$_""" } else { "$_" } }) -join ' '
    $combined = & cmd /c "`"$exe`" $argStr 2>&1"
    $ec = $LASTEXITCODE
    if ($ec -eq 0) { throw "should have failed (exit 0)" }
    return ($combined -join "`n")
}

Write-Host "=== Windows Integration Tests ==="
Write-Host ""

# Test 1: brick --version
Write-Host "--- CLI ---"
Test-Command "brick --version" {
    Exec-Check $brick @("--version")
}

# Test 2: brick build hello.brc
Write-Host "--- brick build ---"
$helloBrc = Join-Path $rootDir "examples\hello.brc"
$outExe = Join-Path $env:TEMP "brick_test_hello_$PID.exe"
Test-Command "brick build hello.brc" {
    Remove-Item -LiteralPath $outExe -ErrorAction SilentlyContinue
    Exec-Check $brick @("build", $helloBrc, "-o", $outExe)
    if (-not (Test-Path $outExe)) { throw "output exe not created" }
}

# Test 3: Run the built exe
Write-Host "--- brick build output run ---"
Test-Command "run built exe" {
    $text = Exec-Check $outExe @()
    if ($text -notmatch "Hello from Brick") { throw "unexpected output: $text" }
    Remove-Item -LiteralPath $outExe -ErrorAction SilentlyContinue
}

# Test 4: brick run hello.brc
Write-Host "--- brick run ---"
Test-Command "brick run hello.brc" {
    $text = Exec-Check $brick @("run", $helloBrc)
    if ($text -notmatch "Hello from Brick") { throw "unexpected output: $text" }
}

# Test 5: .brc file with errors should fail
Write-Host "--- Error handling ---"
$badBrc = Join-Path $env:TEMP "brick_test_error_$PID.brc"
Test-Command "brick build with syntax error" {
    Set-Content -LiteralPath $badBrc -Value "package BAD`nfn main() {`n    bad syntax here`n}"
    Exec-Check-Should-Fail $brick @("build", $badBrc, "-o", "$env:TEMP\brick_test_bad_$PID.exe")
    Remove-Item -LiteralPath $badBrc -ErrorAction SilentlyContinue
}

# Test 6: brick build c_math_test.brc (C math interop)
Write-Host "--- C math interop ---"
$mathBrc = Join-Path $rootDir "examples\c_math_test.brc"
if (Test-Path $mathBrc) {
    Test-Command "brick build c_math_test.brc" {
        Remove-Item -LiteralPath "$env:TEMP\brick_test_math_$PID.exe" -ErrorAction SilentlyContinue
        Exec-Check $brick @("build", $mathBrc, "-o", "$env:TEMP\brick_test_math_$PID.exe")
    }
}

Write-Host ""
Write-Host "=== Results: $passed passed, $failed failed ==="
exit $(if ($failed -gt 0) { 1 } else { 0 })
