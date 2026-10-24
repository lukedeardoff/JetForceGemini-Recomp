# Builds the live map (with the "Warp to exits" door warp) as a standalone
# tool for community builds: %LOCALAPPDATA%\JFGRecomp\tools\JFG-Map.exe.
# The official launcher hosts this map, but community\play-community.ps1 starts
# the game without the launcher, so the map is built on its own here from the
# same launcher sources (no Visual Studio needed, only the .NET Framework
# compiler that ships with Windows). play-community.ps1 opens it automatically.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $csc)) { throw 'The Windows .NET Framework compiler (csc.exe) is missing.' }
$out = Join-Path $env:LOCALAPPDATA 'JFGRecomp\tools'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$commit = (& git -C $repo rev-parse HEAD).Trim()
$info = Join-Path $out 'BuildInfo.cs'
[IO.File]::WriteAllText($info, 'namespace JfgLauncher { internal static class BuildInfo { internal const string SourceCommit = "' + $commit + '"; } }')
$windows = Join-Path $repo 'launcher\windows'
$sources = @('Launcher.cs', 'FirstRun.cs', 'UpdateChecker.cs', 'PortablePackage.cs', 'ImportedRom.cs',
    'FrontendBridge.cs', 'Support.cs', 'Diagnostics.cs', 'Controller.cs', 'NavigationMap.cs', 'Audio.cs',
    'MapLayers.cs', 'NavigationRoute.cs', 'NavigationExplorer.cs', 'NavigationCollision.cs', 'BoxJump.cs',
    'ChestRoute.cs', 'NavigationRunner.cs', 'AutonomousExplorer.cs', 'InventoryWindow.cs',
    'InventoryImages.cs', 'InventoryModel.cs', 'NativeLiveTools.cs') | ForEach-Object { Join-Path $windows $_ }
$icon = Join-Path $repo 'launcher\ui\assets\app.ico'
$font = Join-Path $repo 'launcher\ui\fonts\Barlow-Regular.ttf'
$setup = Join-Path $windows 'Setup.ps1'
$exe = Join-Path $out 'JFG-Map.exe'
& $csc "/win32icon:$icon" "/resource:$icon,JfgLauncher.AppIcon" '/nologo' '/optimize+' '/debug-' '/platform:x64' `
    '/reference:System.Windows.Forms.dll' '/reference:System.Drawing.dll' `
    '/reference:System.Runtime.Serialization.dll' '/reference:System.IO.Compression.dll' `
    "/resource:$setup,JfgLauncher.Setup.ps1" "/resource:$font,JfgLauncher.Barlow.Regular" `
    '/target:winexe' "/out:$exe" @sources $info
if ($LASTEXITCODE -ne 0) { throw 'The map tool did not compile.' }
Write-Host "Map tool built: $exe" -ForegroundColor Green
