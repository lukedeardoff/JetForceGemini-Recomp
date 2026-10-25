# Run the newest successful build of this checkout with the community settings.
$ErrorActionPreference = 'Stop'
# Only this checkout's build cache (same key as scripts/build_paths.py), so
# other checkouts such as pr-shadow are never picked up here.
function Get-CacheKey($folder) {
    $full = (Resolve-Path $folder).Path.ToLowerInvariant()
    $hash = [Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($full))
    return ([BitConverter]::ToString($hash).Replace('-', '').ToLowerInvariant()).Substring(0, 12)
}
$cacheRoot = Join-Path (Join-Path $env:LOCALAPPDATA 'JFG\b') (Get-CacheKey (Split-Path $PSScriptRoot))
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
# Keep a copy of the save from before each session (newest 30), so progress can be rewound.
$saveFile = Join-Path $p 'jfg.flash'
if (Test-Path $saveFile) {
    $backups = Join-Path $p 'backups'
    New-Item -ItemType Directory -Force -Path $backups | Out-Null
    Copy-Item $saveFile (Join-Path $backups ("jfg.flash." + (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss') + ".bak"))
    Get-ChildItem $backups -Filter 'jfg.flash.*.bak' | Sort-Object Name -Descending |
        Select-Object -Skip 30 | Remove-Item -ErrorAction SilentlyContinue
}
# Test settings set earlier in this window stay on until the window closes.
$leftover = Get-ChildItem Env: | Where-Object {
    $_.Name -like 'JFG_*' -and $_.Name -notin @('JFG_PHASE9_RENDERER_WRITEBACK_PROBE', 'JFG_RT64_DEVELOPER', 'JFG_LIVE_OUTPUT') }
if ($leftover) {
    Write-Host ("Test settings still on in this window: " +
        (($leftover | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join ', ') +
        ". Open a new PowerShell window for a normal game.") -ForegroundColor Yellow
}
# Live map with the door warp (Mods > Warp to exits, then double-click a door).
# Like the official launcher, each run exports to a new folder; the map's mod
# choices are kept in map-settings (door warp on by default, cheats off).
$live = Join-Path $p ("maps\community-" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force -Path $live | Out-Null
$modSettings = Join-Path $p 'map-settings'
New-Item -ItemType Directory -Force -Path $modSettings | Out-Null
if (-not (Test-Path (Join-Path $modSettings 'mods.txt'))) {
    [IO.File]::WriteAllText((Join-Path $modSettings 'mods.txt'), 'JFGMODS1 1 0 0')
}
Copy-Item (Join-Path $modSettings 'mods.txt') (Join-Path $live 'mods.txt') -Force
$env:JFG_LIVE_OUTPUT = $live
$mapTool = Join-Path $env:LOCALAPPDATA 'JFGRecomp\tools\JFG-Map.exe'
if (Test-Path $mapTool) {
    Start-Process -FilePath $mapTool -ArgumentList @('--community-map', ('"' + $live + '"'))
} else {
    Write-Host "No map tool yet: run community\build-map-tool.ps1 once for the door warp." -ForegroundColor Yellow
}
Write-Host "Running $exe" -ForegroundColor Cyan
Push-Location (Split-Path $exe)
try { & $exe --rom $rom --save "$p\jfg.flash" --controller-pak "$p\controller-1.pak" --play } finally { Pop-Location }
