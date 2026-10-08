# Launch Jet Force Gemini native build with diagnostic switches.
$env:JFG_PHASE9_RENDERER_WRITEBACK_PROBE = "1"
$env:JFG_RT64_DEVELOPER = "1"
$env:JFG_SHADOW_BLUR = "1.5"
$env:JFG_TEST_NO_TEXSHIFT = "1"
# Asteroid test A: normal texture shifts, writes diag\texlog.tsv
$env:JFG_CAPTURE_EVERY = "0"
$env:JFG_CAPTURE_FROM_S = "30"
$env:JFG_CAPTURE_TO_S = "56"
$env:JFG_EFFECT_CAPTURE = "0"
$env:JFG_TEXLOG = "1"
$env:JFG_TEXSHIFT_MODE = "0"
$exe = Join-Path $PSScriptRoot "tools\private\local-builds\20261002-231450\native\Release\jfg-native-boot.exe"
$p = Join-Path $env:LOCALAPPDATA "JFGRecomp\profiles\default"
$rom = "C:\Users\Owner\Downloads\Jet Force Gemini (USA).z64"
Push-Location (Split-Path $exe)
try {
& $exe --rom $rom --save "$p\jfg.flash" --controller-pak "$p\controller-1.pak" --play
} finally { Pop-Location }
