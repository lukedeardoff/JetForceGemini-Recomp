[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Runtime,
    [Parameter(Mandatory=$true)][string]$Rom,
    [string]$Save,
    [string]$ControllerPak,
    [string]$ProfileDirectory,
    [string]$RunDirectory,
    [ValidateScript({ $_ -eq 0 -or ($_ -ge 60 -and $_ -le 1000) })]
    [int]$Fps = 60,
    [switch]$Unlimited,
    [ValidateSet('On','Off')][string]$VSync = 'On'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$Runtime = (Resolve-Path -LiteralPath $Runtime).Path
$Rom = (Resolve-Path -LiteralPath $Rom).Path
if ($Save) { $Save = (Resolve-Path -LiteralPath $Save).Path }
if ($ControllerPak) { $ControllerPak = (Resolve-Path -LiteralPath $ControllerPak).Path }
if ($ProfileDirectory) { $ProfileDirectory = (Resolve-Path -LiteralPath $ProfileDirectory).Path }
if (-not $RunDirectory) {
    $RunDirectory = Join-Path (Split-Path -Parent $Runtime) (
        'preview-runs\' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
}
if (Test-Path -LiteralPath $RunDirectory) { throw "Preview directory already exists: $RunDirectory" }
$RunDirectory = [System.IO.Directory]::CreateDirectory($RunDirectory).FullName
if ($Save) { Copy-Item -LiteralPath $Save -Destination (Join-Path $RunDirectory 'jfg.flash') }
if ($ControllerPak) { Copy-Item -LiteralPath $ControllerPak -Destination (Join-Path $RunDirectory 'controller.pak') }
if ($ProfileDirectory) {
    foreach ($name in @('controller.ini','modern-controls.ini','experimental-controls.ini','pc-input.ini','audio.ini','video.ini')) {
        $source = Join-Path $ProfileDirectory $name
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            Copy-Item -LiteralPath $source -Destination (Join-Path $RunDirectory $name)
        }
    }
}
# Keep preview persistence and environment separate from installed play.
$savedEnvironment = @{}
Get-ChildItem Env:JFG_* | ForEach-Object {
    $savedEnvironment[$_.Name] = $_.Value
    Remove-Item -LiteralPath ('Env:' + $_.Name)
}
$exitCode = 1
try {
    $env:JFG_EXPERIMENTAL_RENDER_FPS = if ($Unlimited) { 'unlimited' } else { [string]$Fps }
    $env:JFG_RENDER_VSYNC = if ($VSync -eq 'On') { '1' } else { '0' }
    $env:JFG_VIDEO_CONFIG = Join-Path $RunDirectory 'video.ini'
    $env:JFG_CONTROLLER_CONFIG = Join-Path $RunDirectory 'controller.ini'
    $env:JFG_MASTER_VOLUME_CONFIG = Join-Path $RunDirectory 'audio.ini'
    Write-Host "Rendering target: $env:JFG_EXPERIMENTAL_RENDER_FPS; VSync: $VSync."
    Write-Host "Preview save and settings: $RunDirectory"
    Push-Location -LiteralPath $RunDirectory
    try {
        & $Runtime --rom $Rom --save (Join-Path $RunDirectory 'jfg.flash') --controller-pak (Join-Path $RunDirectory 'controller.pak') --play
        $exitCode = $LASTEXITCODE
    }
    finally { Pop-Location }
}
finally {
    Get-ChildItem Env:JFG_* | ForEach-Object { Remove-Item -LiteralPath ('Env:' + $_.Name) }
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
exit $exitCode
