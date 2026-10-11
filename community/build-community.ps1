# Full build of this checkout from your ROM (normal upstream pipeline).
# Reuses an existing RT64/dependency folder that already has the community
# RT64 patches applied, so the shadow soften pass and colour grade compile in.
$rom = "C:\Users\Owner\Downloads\Jet Force Gemini (USA).z64"
$root = Split-Path $PSScriptRoot
$deps = Get-ChildItem (Join-Path $env:LOCALAPPDATA 'JFG\b') -Directory -ErrorAction SilentlyContinue |
    ForEach-Object { Join-Path $_.FullName 'd' } |
    Where-Object { Test-Path (Join-Path $_ 'rt64\src\shaders\FbShadowSoftenCS.hlsl') } | Select-Object -First 1
Push-Location $root
try {
    if ($deps) {
        $rt64 = Join-Path $deps 'rt64'
        if ((Select-String -Path (Join-Path $rt64 'src\shaders\VideoInterfacePS.hlsl') -Pattern 'jfgContrast' -Quiet) -ne $true) {
            git -C $rt64 apply (Join-Path $root 'patches\rt64\jfg-color-grade.patch')
        }
        Write-Host "Using patched dependencies in $deps" -ForegroundColor Cyan
        python scripts\build_from_rom.py --rom $rom --dependency-root $deps
    } else {
        Write-Host "No patched dependency folder yet: run this once, then rebuild-community.ps1 (it applies the RT64 patches)." -ForegroundColor Yellow
        python scripts\build_from_rom.py --rom $rom
    }
} finally { Pop-Location }
