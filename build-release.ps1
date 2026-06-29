param(
    [switch]$skipVsix = $false
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ReleaseDir = Join-Path $ScriptDir "build\release"
$BuildDir = Join-Path $ScriptDir "build"

Write-Host "╔═══════════════════════════════════════════════╗"
Write-Host "║     Brick Release Builder v0.3 (Windows)   ║"
Write-Host "╚═══════════════════════════════════════════════╝"

# ─── 1. Build release profile ──────────────────────────────
Write-Host ""
Write-Host "[1/3] Building brick (release)..."
Set-Location $ScriptDir
scons profile=release -j $env:NUMBER_OF_PROCESSORS
if ($LASTEXITCODE -ne 0) { throw "SCons build failed" }

# ─── 2. Assemble release ────────────────────────────────────
Write-Host ""
Write-Host "[2/3] Assembling release..."
if (Test-Path $ReleaseDir) { Remove-Item -Recurse -Force $ReleaseDir }
New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null

Copy-Item (Join-Path $BuildDir "brick.exe") (Join-Path $ReleaseDir "brick.exe")
Write-Host "      brick.exe"

# ─── 3. Package VS Code extension ──────────────────────────
if (-not $skipVsix) {
    Write-Host ""
    Write-Host "[3/3] Packaging VS Code extension..."
    $VscePath = Join-Path $ReleaseDir "brick-language.vsix"
    $VsCodeExt = Join-Path $ScriptDir "vscode-ext"

    if (Test-Path $VsCodeExt) {
        Write-Host "      Compiling extension..."
        Set-Location $VsCodeExt
        npm install 2>&1 | ForEach-Object { "      $_" }
        npm run compile 2>&1 | ForEach-Object { "      $_" }

        $ServerDir = Join-Path $VsCodeExt "server"
        if (Test-Path $ServerDir) {
            Set-Location $ServerDir
            npm install 2>&1 | ForEach-Object { "      $_" }
            npm run compile 2>&1 | ForEach-Object { "      $_" }
            Set-Location $ScriptDir
        }

        Write-Host "      Packaging .vsix..."
        npx --yes vsce package --out $VscePath 2>&1 | ForEach-Object { "      $_" }

        if (Test-Path $VscePath) {
            $size = (Get-Item $VscePath).Length / 1MB
            Write-Host "      brick-language.vsix  ($([math]::Round($size, 1)) MB)"
        } else {
            Write-Host "      ERROR: .vsix was not created!"
            exit 1
        }
    }
} else {
    Write-Host ""
    Write-Host "[3/3] Skipping VS Code extension packaging"
}

Write-Host ""
Write-Host ""
Write-Host "═══════════════════════════════════════════════════"
Write-Host "  Release ready: $ReleaseDir"
Write-Host "    brick.exe          (compiler + visualizer)"
if (-not $skipVsix -and (Test-Path $VscePath)) {
    Write-Host "    brick-language.vsix (VS Code extension)"
}
Write-Host "═══════════════════════════════════════════════════"
