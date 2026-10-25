# Rebuild the newest build of this checkout after code changes (fast, no ROM regeneration).
# Also makes sure the community shadow patch is applied to the RT64 dependency.
$rom = "C:\Users\Owner\Downloads\Jet Force Gemini (USA).z64"
$root = Split-Path $PSScriptRoot
# Only this checkout's build cache (same key as scripts/build_paths.py), so
# other checkouts such as pr-shadow are never picked up here.
function Get-CacheKey($folder) {
    $full = (Resolve-Path $folder).Path.ToLowerInvariant()
    $hash = [Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($full))
    return ([BitConverter]::ToString($hash).Replace('-', '').ToLowerInvariant()).Substring(0, 12)
}
$cacheRoot = Join-Path (Join-Path $env:LOCALAPPDATA 'JFG\b') (Get-CacheKey $root)
$ws = Get-ChildItem (Join-Path $cacheRoot 'w') -Directory -ErrorAction SilentlyContinue |
    Where-Object { Test-Path (Join-Path $_.FullName 'generation.json') } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $ws) { Write-Host "No build found. Run build-community.ps1 first." -ForegroundColor Red; exit 1 }
$rt64 = Join-Path (Split-Path (Split-Path $ws.FullName)) 'd\rt64'
$patched = $false
if (-not (Test-Path (Join-Path $rt64 'src\shaders\FbShadowSoftenCS.hlsl'))) {
    git -C $rt64 apply (Join-Path $root 'patches\rt64\jfg-shadow-soften.patch')
    if ($LASTEXITCODE -ne 0) { Write-Host "Failed to apply jfg-shadow-soften.patch" -ForegroundColor Red; exit 1 }
    $patched = $true
}
if ((Select-String -Path (Join-Path $rt64 'src\shaders\VideoInterfacePS.hlsl') -Pattern 'jfgContrast' -Quiet) -ne $true) {
    git -C $rt64 apply (Join-Path $root 'patches\rt64\jfg-color-grade.patch')
    if ($LASTEXITCODE -ne 0) { Write-Host "Failed to apply jfg-color-grade.patch" -ForegroundColor Red; exit 1 }
    $patched = $true
}
# MSBuild's header tracking misses RT64's shared CPU/HLSL headers, so a patch that
# changes a struct such as VideoInterfaceCB can leave stale objects with the old
# layout (root signature too small -> null PSO -> crash in D3D12Core). After any
# patch, touch every RT64 source so the whole library recompiles once.
if ($patched) {
    $now = Get-Date
    Get-ChildItem (Join-Path $rt64 'src') -Recurse -Include *.cpp, *.c |
        ForEach-Object { $_.LastWriteTime = $now }
}
Push-Location $root
try { python scripts\build_from_rom.py --rom $rom --resume-native $ws.FullName } finally { Pop-Location }
