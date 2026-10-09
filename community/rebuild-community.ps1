# Rebuild the newest build of this checkout after code changes (fast, no ROM regeneration).
# Also makes sure the community shadow patch is applied to the RT64 dependency.
$rom = "C:\Users\Owner\Downloads\Jet Force Gemini (USA).z64"
$root = Split-Path $PSScriptRoot
$cacheRoot = Join-Path $env:LOCALAPPDATA 'JFG\b'
$ws = Get-ChildItem $cacheRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object { Get-ChildItem (Join-Path $_.FullName 'w') -Directory -ErrorAction SilentlyContinue } |
    Where-Object { Test-Path (Join-Path $_.FullName 'generation.json') } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $ws) { Write-Host "No build found. Run build-community.ps1 first." -ForegroundColor Red; exit 1 }
$rt64 = Join-Path (Split-Path (Split-Path $ws.FullName)) 'd\rt64'
if (-not (Test-Path (Join-Path $rt64 'src\shaders\FbShadowSoftenCS.hlsl'))) {
    git -C $rt64 apply (Join-Path $root 'patches\rt64\jfg-shadow-soften.patch')
}
Push-Location $root
try { python scripts\build_from_rom.py --rom $rom --resume-native $ws.FullName } finally { Pop-Location }
