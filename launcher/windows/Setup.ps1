[CmdletBinding()]
param(
    [string]$RomPath,
    [ValidatePattern('^[0-9a-f]{40}$')][string]$SourceCommit,
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$script:SetupRoot = Join-Path $env:LOCALAPPDATA 'JFGRecomp'
$script:Distro = 'Ubuntu-24.04'
$script:LinuxPackages = @('build-essential', 'gcc-multilib', 'binutils-mips-linux-gnu',
    'python3-venv', 'python3-pip', 'pkg-config', 'cmake', 'ninja-build', 'git', 'wget')

function Refresh-SetupPath {
    $env:PATH = [Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
        [Environment]::GetEnvironmentVariable('PATH', 'User')
}

function Find-Git {
    $command = Get-Command git.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $path = Join-Path $env:ProgramFiles 'Git\cmd\git.exe'
    if (Test-Path -LiteralPath $path -PathType Leaf) { return $path }
    return $null
}

function Find-Python {
    $candidates = @()
    $command = Get-Command python.exe -ErrorAction SilentlyContinue
    # The WindowsApps alias can be an installed Store Python or an empty Store
    # shortcut. Only invoke it when the Python package actually exists.
    $storePython = @(Get-AppxPackage -Name 'PythonSoftwareFoundation.Python.*' -ErrorAction SilentlyContinue)
    if ($command -and ($command.Source -notlike '*\WindowsApps\*' -or $storePython.Count -gt 0)) {
        $candidates += $command.Source
    }
    foreach ($version in @('314', '313', '312', '311')) {
        $candidates += Join-Path $env:LOCALAPPDATA "Programs\Python\Python$version\python.exe"
        $candidates += Join-Path $env:ProgramFiles "Python$version\python.exe"
    }
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            & $candidate -c 'import sys; sys.exit(0 if sys.version_info >= (3,11) and sys.maxsize > 2**32 else 1)' 2>$null
            if ($LASTEXITCODE -eq 0) { return $candidate }
        }
    }
    return $null
}

function Find-VisualStudio {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { return $null }
    $found = @(& $vswhere -latest -products '*' -version '[17.0,18.0)' -requires `
        Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.CMake.Project `
        -property installationPath)
    if ($LASTEXITCODE -eq 0 -and $found.Count -gt 0) { return $found[0] }
    return $null
}

function Invoke-SetupProbe([string]$File, [string[]]$Arguments) {
    # Missing prerequisites may write native stderr. They are a negative probe,
    # not a fatal PowerShell error under the installer's Stop preference.
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $lines = @(& $File @Arguments 2>$null)
        $probeExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    return [pscustomobject]@{ ExitCode = $probeExit; Lines = $lines }
}

function Test-Ubuntu {
    $wsl = Join-Path $env:WINDIR 'System32\wsl.exe'
    if (-not (Test-Path -LiteralPath $wsl)) { return $false }
    $probe = Invoke-SetupProbe $wsl @('--list', '--quiet')
    $installed = ($probe.Lines -join "`n").Replace([string][char]0, '')
    return ($probe.ExitCode -eq 0 -and @($installed -split "`r?`n" | Where-Object { $_.Trim() -eq $script:Distro }).Count -gt 0)
}

function Test-LinuxPackages {
    if (-not (Test-Ubuntu)) { return $false }
    $probe = Invoke-SetupProbe "$env:WINDIR\System32\wsl.exe" (@('-d', $script:Distro, '-u', 'root', '--exec',
        'dpkg-query', '-W', '-f=${db:Status-Abbrev}\n') + $script:LinuxPackages)
    $lines = @($probe.Lines)
    return ($probe.ExitCode -eq 0 -and $lines.Count -eq $script:LinuxPackages.Count -and
        @($lines | Where-Object { $_ -notmatch '^ii' }).Count -eq 0)
}

function Invoke-Checked([string]$File, [string[]]$Arguments) {
    $tool = [IO.Path]::GetFileNameWithoutExtension($File).ToLowerInvariant()
    if ($tool -in @('git', 'python', 'python3', 'winget', 'wsl')) { Write-Output "JFG-SUPPORT setup_step=$tool" }
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $File @Arguments 2>&1 | ForEach-Object { Write-Output ([string]$_) }
        $commandExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($null -eq $commandExit) { throw 'Setup command did not start.' }
    Write-Output ('JFG-SUPPORT setup_exit=0x{0:x8}' -f $commandExit)
    if ($commandExit -in @(3010, 1641)) {
        Write-Output 'JFG-RESTART: Restart Windows, reopen this launcher and click Set up and build again.'
        exit 3010
    }
    if ($commandExit -ne 0) { throw "Setup command failed ($commandExit): $([IO.Path]::GetFileName($File)). See setup.log." }
}

function Install-Package([string]$Id, [string]$Override = '') {
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
    if (-not $winget) { throw 'Install or update Microsoft App Installer from Microsoft Store, then reopen the launcher. Setup requires winget.' }
    Write-Output "Installing $Id. Approve the Windows installer prompt if shown."
    $arguments = @('install', '--id', $Id, '--exact', '--source', 'winget',
        '--accept-package-agreements', '--accept-source-agreements', '--disable-interactivity')
    if ($Override) { $arguments += @('--override', $Override) }
    Invoke-Checked $winget.Source $arguments
    Refresh-SetupPath
}

function Get-VirtualizationFirmwareState {
    try {
        $computer = Get-CimInstance -ClassName Win32_ComputerSystem -ErrorAction Stop
        # An already running hypervisor is usable even when a VM hides the CPU flag.
        if ($computer.HypervisorPresent) { return 'enabled' }
        $processors = @(Get-CimInstance -ClassName Win32_Processor -ErrorAction Stop)
        $known = @($processors | Where-Object { $null -ne $_.VirtualizationFirmwareEnabled })
        if (@($known | Where-Object { -not $_.VirtualizationFirmwareEnabled }).Count -gt 0) { return 'disabled' }
        if ($known.Count -gt 0) { return 'enabled' }
    } catch { }
    return 'unknown'
}

function Assert-WslVirtualization {
    if ((Get-VirtualizationFirmwareState) -eq 'disabled') {
        throw 'Hardware virtualization is disabled in firmware. Enable Intel VT-x or AMD SVM in BIOS/UEFI before installing WSL. Restarting Windows alone will not fix this. The prebuilt game package does not require WSL.'
    }
}

function Get-WslOnlineDistributions {
    $wsl = Join-Path $env:WINDIR 'System32\wsl.exe'
    if (-not (Test-Path -LiteralPath $wsl -PathType Leaf)) {
        return [pscustomobject]@{ ExitCode = -1; Lines = @() }
    }
    return Invoke-SetupProbe $wsl @('--list', '--online')
}

function Test-RequestedDistribution($Probe) {
    $text = ($Probe.Lines -join "`n").Replace([string][char]0, '')
    $name = [regex]::Escape($script:Distro)
    return ($Probe.ExitCode -eq 0 -and $text -match ('(?m)^\s*\*?\s*' + $name + '(?:\s|$)'))
}

function Invoke-WslDistributionInstall {
    $process = Start-Process -FilePath "$env:WINDIR\System32\wsl.exe" `
        -ArgumentList "--install -d $script:Distro --no-launch --web-download" `
        -Verb RunAs -WindowStyle Hidden -Wait -PassThru
    return $process.ExitCode
}

function Ensure-Ubuntu {
    if (Test-Ubuntu) { return }
    Assert-WslVirtualization
    if (-not (Test-RequestedDistribution (Get-WslOnlineDistributions))) {
        Write-Output 'Installing or updating Microsoft WSL to support Ubuntu 24.04...'
        Install-Package 'Microsoft.WSL'
        if (-not (Test-RequestedDistribution (Get-WslOnlineDistributions))) {
            throw 'WSL does not list Ubuntu-24.04 after its update. Run wsl --update, then wsl --list --online. Check network access and Windows updates before retrying. No distribution was installed; existing distributions are preserved.'
        }
    }
    Write-Output 'Installing WSL and Ubuntu 24.04. Approve the Windows prompt if shown.'
    $installExit = Invoke-WslDistributionInstall
    Write-Output ('JFG-SUPPORT setup_exit=0x{0:x8}' -f $installExit)
    if ($installExit -in @(3010, 1641)) {
        Write-Output 'JFG-RESTART: The installer requested a Windows restart. Reopen the launcher afterward.'
        exit 3010
    }
    if ($installExit -ne 0) {
        throw "WSL installation failed ($installExit). See setup.log and check wsl --status. This is not a confirmed restart requirement."
    }
    if (-not (Test-Ubuntu)) {
        throw 'WSL returned success but Ubuntu-24.04 is not registered. Check wsl --status and Windows feature requirements; restart only if Windows requested it, then retry. See setup.log.'
    }
}

function Ensure-BuildTools {
    if (-not (Test-Ubuntu)) { Assert-WslVirtualization }
    if (-not (Find-Git)) { Install-Package 'Git.Git' }
    if (-not (Find-Python)) { Install-Package 'Python.Python.3.12' }
    if (-not (Find-VisualStudio)) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        $existing = @()
        if (Test-Path -LiteralPath $vswhere) {
            $existing = @(& $vswhere -latest -products '*' -version '[17.0,18.0)' -property installationPath)
        }
        if ($existing.Count -gt 0) {
            Write-Output 'Adding C++ and CMake to Visual Studio 2022. Approve the Windows prompt.'
            $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
            $product = @(& $vswhere -latest -products '*' -version '[17.0,18.0)' -property productId)
            $workload = if ($product -contains 'Microsoft.VisualStudio.Product.BuildTools') { 'VCTools' } else { 'NativeDesktop' }
            $arguments = 'modify --installPath "' + $existing[0] + '" --add Microsoft.VisualStudio.Workload.' + $workload + ' --includeRecommended --passive --norestart'
            $process = Start-Process -FilePath $installer -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -Wait -PassThru
            if ($process.ExitCode -in @(3010, 1641)) { Write-Output 'JFG-RESTART: Restart Windows, reopen the launcher and continue setup.'; exit 3010 }
            if ($process.ExitCode -ne 0) { throw 'Visual Studio setup did not finish. Reopen the launcher to retry.' }
        } else {
            Install-Package 'Microsoft.VisualStudio.2022.BuildTools' '--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
        }
    }
    if (-not (Find-Git) -or -not (Find-Python) -or -not (Find-VisualStudio)) {
        throw 'Build tools are incomplete. Restart Windows after any installer, then reopen the launcher.'
    }
    Ensure-Ubuntu
    if (-not (Test-LinuxPackages)) {
        Write-Output 'Installing the Ubuntu build packages...'
        $wsl = "$env:WINDIR\System32\wsl.exe"
        Invoke-Checked $wsl @('-d', $script:Distro, '-u', 'root', '--exec', 'apt-get', 'update')
        Invoke-Checked $wsl (@('-d', $script:Distro, '-u', 'root', '--exec', 'env', 'DEBIAN_FRONTEND=noninteractive', 'apt-get', 'install', '-y') + $script:LinuxPackages)
        if (-not (Test-LinuxPackages)) { throw 'Ubuntu build packages are incomplete. See setup.log and retry setup.' }
    }
}

function Get-PinnedSource([string]$Commit) {
    if ($Commit -notmatch '^[0-9a-f]{40}$') { throw 'The launcher has no valid source revision.' }
    $git = Find-Git
    $cache = [IO.Path]::GetFullPath((Join-Path $script:SetupRoot 'source'))
    $destination = Join-Path $cache $Commit
    if (-not (Test-Path -LiteralPath $destination)) {
        New-Item -ItemType Directory -Path $cache -Force | Out-Null
        $temporary = Join-Path $cache ('download-' + [Guid]::NewGuid().ToString('N'))
        Write-Output 'Downloading the source version bundled with this launcher...'
        Invoke-Checked $git @('init', $temporary)
        Invoke-Checked $git @('-C', $temporary, 'config', 'core.autocrlf', 'false')
        Invoke-Checked $git @('-C', $temporary, 'config', 'core.longpaths', 'true')
        Invoke-Checked $git @('-C', $temporary, 'remote', 'add', 'origin', 'https://github.com/TK22-26/JetForceGemini-Recomp.git')
        Invoke-Checked $git @('-C', $temporary, 'fetch', '--depth=1', 'origin', $Commit)
        Invoke-Checked $git @('-C', $temporary, 'checkout', '--detach', $Commit)
        # Both resolved paths must stay inside the explicitly owned source cache.
        foreach ($path in @($temporary, $destination)) {
            if (-not [IO.Path]::GetFullPath($path).StartsWith($cache + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Source cache path escaped.' }
        }
        Move-Item -LiteralPath $temporary -Destination $destination
    }
    $head = @(& $git -C $destination rev-parse HEAD)
    if ($LASTEXITCODE -ne 0 -or $head.Count -ne 1 -or $head[0] -ne $Commit) { throw 'Cached source revision differs. Existing files were preserved.' }
    $changes = @(& $git -C $destination status --porcelain)
    if ($LASTEXITCODE -ne 0 -or $changes.Count -ne 0) { throw 'Cached source has local changes. Existing files were preserved.' }
    if (-not (Test-Path -LiteralPath (Join-Path $destination 'scripts\build_from_rom.py'))) { throw 'Downloaded source is incomplete.' }
    return $destination
}

function Invoke-Setup {
    if ($CheckOnly) {
        [ordered]@{ git = [bool](Find-Git); python = [bool](Find-Python);
            visualStudio = [bool](Find-VisualStudio); ubuntu = Test-Ubuntu;
            linuxPackages = Test-LinuxPackages } | ConvertTo-Json -Compress
        return
    }
    if (-not $SourceCommit -or -not $RomPath) { throw 'Choose a ROM through the launcher.' }
    # Direct invocation has the same no-download-before-ROM-validation boundary.
    if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf) -or (Get-Item -LiteralPath $RomPath).Length -ne 33554432 -or
        (Get-FileHash -LiteralPath $RomPath -Algorithm SHA1).Hash -ne '493ced9008dbe932d6e91179b68e8630cf23a023') {
        throw 'Select the supported 32 MiB North American big-endian ROM.'
    }
    New-Item -ItemType Directory -Path $script:SetupRoot -Force | Out-Null
    Start-Transcript -Path (Join-Path $script:SetupRoot 'setup.log') -Append | Out-Null
    try {
        Write-Output 'JFG-SUPPORT stage=install-tools'
        Ensure-BuildTools
        # Keep status output visible without mixing it into the returned path.
        Write-Output 'JFG-SUPPORT stage=download-source'
        $source = Get-PinnedSource $SourceCommit | ForEach-Object {
            if ($_ -eq (Join-Path (Join-Path $script:SetupRoot 'source') $SourceCommit)) { $_ }
            else { Write-Host $_ }
        }
        $python = Find-Python
        Write-Output 'JFG-SUPPORT stage=build-game'
        Write-Output 'Building the game locally from your ROM...'
        Invoke-Checked $python @('-u', (Join-Path $source 'scripts\build_from_rom.py'), '--rom', $RomPath)
    } finally { Stop-Transcript | Out-Null }
}

if ($MyInvocation.InvocationName -ne '.') {
    try { Invoke-Setup }
    catch {
        Write-Output ('JFG-SUPPORT setup_error=' + $_.CategoryInfo.Category.ToString().ToLowerInvariant())
        [Console]::Error.WriteLine($_.Exception.Message); exit 1
    }
}
