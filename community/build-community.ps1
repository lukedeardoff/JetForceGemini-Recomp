# Build this checkout from your ROM (normal upstream pipeline). Run from anywhere.
$rom = "C:\Users\Owner\Downloads\Jet Force Gemini (USA).z64"
Push-Location (Split-Path $PSScriptRoot)
try { python scripts\build_from_rom.py --rom $rom } finally { Pop-Location }
