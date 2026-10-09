# Run the newest successful build of this checkout with the community settings.
$ErrorActionPreference = 'Stop'
$cacheRoot = Join-Path $env:LOCALAPPDATA 'JFG\b'
$status = Get-ChildItem $cacheRoot -Recurse -Filter status.json -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Where-Object { $j = Get-Content $_.FullName -Raw | ConvertFrom-Json; $j.complete -and (Test-Path $j.executable) } |
    Select-Object -First 1
if (-not $status) { Write-Host "No finished build found. Run build-community.ps1 first." -ForegroundColor Red; exit 1 }
$exe = (Get-Content $status.FullName -Raw | ConvertFrom-Json).executable
$env:JFG_PHASE9_RENDERER_WRITEBACK_PROBE = "1"
$env:JFG_RT64_DEVELOPER = "1"
$p = Join-Path $env:LOCALAPPDATA "JFGRecomp\profiles\default"
$rom = "C:\Users\Owner\Downloads\Jet Force Gemini (USA).z64"
Write-Host "Running $exe" -ForegroundColor Cyan
Push-Location (Split-Path $exe)
try { & $exe --rom $rom --save "$p\jfg.flash" --controller-pak "$p\controller-1.pak" --play } finally { Pop-Location }
