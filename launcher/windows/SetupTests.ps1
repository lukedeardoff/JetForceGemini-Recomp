$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'Setup.ps1')
$script:checks = 0
function Assert-Setup($Condition, [string]$Description) {
    if (-not $Condition) { throw $Description }
    $script:checks++
}
$probe = Join-Path ([IO.Path]::GetTempPath()) ('jfg-stderr-' + [Guid]::NewGuid().ToString('N') + '.ps1')
[IO.File]::WriteAllText($probe, "[Console]::Error.WriteLine('fixture native stderr'); exit 7")
try {
    $script:nativeProbe = @()
    $rejected = $false
    try { Invoke-Checked "$env:WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe" @('-NoProfile', '-File', $probe) | ForEach-Object { $script:nativeProbe += $_ } }
    catch { $rejected = $_.Exception.Message -like '*failed (7)*' }
    Assert-Setup $rejected 'Native stderr must preserve the actual command failure.'
    Assert-Setup ($script:nativeProbe -contains 'JFG-SUPPORT setup_exit=0x00000007') 'Native exit code was not included in diagnostics.'
    $negativeProbe = Invoke-SetupProbe "$env:WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe" @('-NoProfile', '-File', $probe)
    Assert-Setup ($negativeProbe.ExitCode -eq 7) 'Missing prerequisite stderr must return its failure status without aborting setup.'
    Assert-Setup (@($negativeProbe.Lines).Count -eq 0) 'Native stderr must not be mistaken for an installed prerequisite.'
    Assert-Setup ($ErrorActionPreference -eq 'Stop') 'A prerequisite probe must restore the installer error preference.'

} finally { Remove-Item -LiteralPath $probe }
function Find-Git { if ($script:hasGit) { 'synthetic-git' } }
function Find-Python { if ($script:hasPython) { 'synthetic-python' } }
function Find-VisualStudio { if ($script:hasVs) { 'synthetic-vs' } }
function Test-Ubuntu { return $true }
function Test-LinuxPackages { return $script:hasLinux }
function Install-Package([string]$Id, [string]$Override = '') {
    $script:installs += $Id
    switch ($Id) {
        'Git.Git' { $script:hasGit = $true }
        'Python.Python.3.12' { $script:hasPython = $true }
        default { throw 'Unexpected installer in fixture' }
    }
}
function Invoke-Checked([string]$File, [string[]]$Arguments) {
    $script:commands += ,$Arguments
    if ($Arguments -contains 'install') { $script:hasLinux = $true }
}
$script:hasGit = $true; $script:hasPython = $true; $script:hasVs = $true; $script:hasLinux = $true
$script:installs = @(); $script:commands = @()
Ensure-BuildTools
Assert-Setup ($script:installs.Count -eq 0 -and $script:commands.Count -eq 0) 'A ready machine must not install anything.'
$script:hasGit = $false; $script:hasPython = $false; $script:hasLinux = $false
Ensure-BuildTools
Assert-Setup (($script:installs -join ',') -eq 'Git.Git,Python.Python.3.12') 'Only missing Windows tools should be installed.'
Assert-Setup ($script:commands.Count -eq 2) 'Ubuntu update/install sequence missing.'
Assert-Setup ($script:commands[0] -contains 'update') 'Ubuntu indexes must be updated first.'
Assert-Setup ($script:commands[1] -contains 'DEBIAN_FRONTEND=noninteractive') 'Ubuntu setup must not await invisible input.'
Assert-Setup ($script:commands[1] -contains 'gcc-multilib') 'Missing matching-ELF build prerequisite.'
$before = $script:installs.Count + $script:commands.Count
Ensure-BuildTools
Assert-Setup (($script:installs.Count + $script:commands.Count) -eq $before) 'Retry must reuse installed components.'
$CheckOnly = $false; $SourceCommit = 'a' * 40
$RomPath = Join-Path ([IO.Path]::GetTempPath()) ('jfg-invalid-' + [Guid]::NewGuid().ToString('N') + '.z64')
[IO.File]::WriteAllBytes($RomPath, [byte[]]@(1,2,3,4))
try {
    $rejected = $false
    try { Invoke-Setup } catch { $rejected = $_.Exception.Message -like '*supported 32 MiB*' }
    Assert-Setup $rejected 'Setup must reject an unsupported ROM before installation.'
    Assert-Setup (($script:installs.Count + $script:commands.Count) -eq $before) 'Bad ROM triggered a download.'
    $rejected = $false
    try { Get-PinnedSource '../main' } catch { $rejected = $_.Exception.Message -like '*source revision*' }
    Assert-Setup $rejected 'Mutable or path-like source revisions must be rejected.'
} finally { Remove-Item -LiteralPath $RomPath }
# Issue #4: exercise firmware, old WSL and failure classifications without
# installing software or changing this machine's Windows/firmware settings.
function Test-Ubuntu { return $script:ubuntuReady }
function Get-VirtualizationFirmwareState { return $script:firmwareState }
function Get-WslOnlineDistributions {
    $lines = if ($script:onlineReady) { @('NAME FRIENDLY NAME', 'Ubuntu-24.04 Ubuntu 24.04 LTS') } else { @('Ubuntu Ubuntu') }
    return [pscustomobject]@{ ExitCode = 0; Lines = $lines }
}
function Install-Package([string]$Id, [string]$Override = '') {
    Assert-Setup ($Id -eq 'Microsoft.WSL') 'Unexpected package in WSL upgrade fixture.'
    $script:wslUpgrades++
    if ($script:upgradeWorks) { $script:onlineReady = $true }
}
function Invoke-WslDistributionInstall {
    $script:wslInstalls++
    if ($script:installWorks) { $script:ubuntuReady = $true }
    return $script:installExit
}
$script:ubuntuReady = $false; $script:firmwareState = 'disabled'
$script:onlineReady = $true; $script:wslUpgrades = 0; $script:wslInstalls = 0
$script:installWorks = $true; $script:installExit = 0; $script:upgradeWorks = $true
$rejected = $false
try { Ensure-Ubuntu } catch { $rejected = $_.Exception.Message -like '*BIOS/UEFI*' }
Assert-Setup $rejected 'Disabled firmware must explain the BIOS prerequisite.'
Assert-Setup ($script:wslUpgrades -eq 0 -and $script:wslInstalls -eq 0) 'Disabled firmware must not start an installation.'
$script:firmwareState = 'enabled'; $script:onlineReady = $false
Ensure-Ubuntu
Assert-Setup ($script:wslUpgrades -eq 1 -and $script:wslInstalls -eq 1 -and $script:ubuntuReady) 'Old WSL must be upgraded before installing the requested distribution.'
Ensure-Ubuntu
Assert-Setup ($script:wslUpgrades -eq 1 -and $script:wslInstalls -eq 1) 'An existing distribution must be reused.'
$script:ubuntuReady = $false; $script:onlineReady = $false; $script:upgradeWorks = $false
$rejected = $false
try { Ensure-Ubuntu } catch { $rejected = $_.Exception.Message -like '*does not list Ubuntu-24.04*' }
Assert-Setup $rejected 'An unsupported distribution after upgrade must be actionable, not a reboot loop.'
Assert-Setup ($script:wslInstalls -eq 1) 'Missing distribution must not invoke an unsupported installation command.'
$script:onlineReady = $true; $script:installWorks = $false; $script:installExit = 23
$rejected = $false
try { Ensure-Ubuntu } catch { $rejected = $_.Exception.Message -like '*failed (23)*' }
Assert-Setup $rejected 'A failed WSL installation must retain its error instead of requesting a restart.'
$script:installExit = 0; $script:firmwareState = 'unknown'
$rejected = $false
try { Ensure-Ubuntu } catch { $rejected = $_.Exception.Message -like '*not registered*' }
Assert-Setup $rejected 'Zero exit with missing Ubuntu must not claim success or loop on reboot.'
Assert-Setup (Test-RequestedDistribution ([pscustomobject]@{ ExitCode = 0; Lines = @("Ubuntu-24.04`0 Ubuntu") })) 'WSL UTF-16 NUL output must be normalized.'
Assert-Setup (-not (Test-RequestedDistribution ([pscustomobject]@{ ExitCode = 0; Lines = @('Ubuntu-24.04-preview Ubuntu') }))) 'A different distribution must not satisfy the requested name.'
Assert-Setup (-not (Test-RequestedDistribution ([pscustomobject]@{ ExitCode = 7; Lines = @('Ubuntu-24.04 Ubuntu') }))) 'A failed catalog probe must not be accepted.'
Write-Output "Setup checks passed: $script:checks"
