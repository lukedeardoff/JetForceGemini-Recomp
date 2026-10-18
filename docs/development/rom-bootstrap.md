# Build the game from your ROM

For downloads and the first-run path, start with [getting started](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/getting-started.md)
and check [known setup issues](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/known-issues.md#windows-10-setup).
The [launcher](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/development/launcher.md)
automates everything below; this page is for manual setup and command-line use.

## Prerequisites

Install Git, Python 3.11 or newer, Visual Studio 2022 with **Desktop development
with C++** and CMake, and WSL2 with Ubuntu 24.04. Inside Ubuntu, install the
upstream build prerequisites:

```sh
sudo apt update
sudo apt install build-essential gcc-multilib binutils-mips-linux-gnu python3-venv python3-pip pkg-config cmake ninja-build git wget
```

## Build and play

From PowerShell at the repository root:

```powershell
python scripts/build_from_rom.py --rom 'D:\Games\my-copy.z64' --play
```

Or run `launcher/windows/Build-And-Play.ps1` to pick the ROM with a file
dialog. The script validates the ROM before downloading anything; other
regions, modified ROMs and byte-swapped dumps are rejected. Compilation takes
several minutes. Saves live in `%LOCALAPPDATA%\JFGRecomp\profiles\default`.

The recipe clones the pinned dependencies, builds a fresh matching ELF in a
private WSL temporary directory, derives overlay boundaries from that build's
linker metadata, applies the pinned N64Recomp patch series, generates CPU and
audio code (the audio configuration comes from the ROM's DMA descriptors and
command table), and builds the Windows runtime. The upstream decompilation is
consulted locally; no upstream game source is copied into this repository. It
never accepts a maintainer-generated ELF, overlay layout, CPU root or audio
source as input.

## Workspaces and options

ROM copies, generated code, logs and binaries stay in the short per-checkout
cache `%LOCALAPPDATA%\JFG\b\<checkout-id>\w\<run-id>` and a private WSL
temporary directory, whose path is recorded in `linux-workspace.txt`. The
command prints the exact workspace. Do not upload either directory or add it
to a release or issue. Each normal run creates a fresh workspace; a failed run
keeps its files.

| Option | Use |
|---|---|
| `--resume-native <workspace>` | Resume after generation succeeded but compilation failed; pass the same `--rom` (and `--build-root`). Manifests and hashes are checked first. |
| `--build-root D:/JFG-builds` | Use a different short, writable cache location. |
| `--dependency-root <dir>` | Reuse an existing pinned upstream source/tool cache (IDO tools, repository objects); the ELF is still rebuilt. Checkouts at another revision are rejected unchanged. |
| `--generation-timeout <s>` | Bound the Linux process group (default one hour). |
| `--jobs <n>` | Build parallelism. |
| `--distro <name>` | Use a different installed WSL distribution. |

Legacy workspaces under `tools/private/local-builds` remain intact and can be
resumed when their native output path fits MSBuild's limit.

## Verification scope

The default path has been verified end to end from the supported ROM: a fresh
ELF, CPU corpus and audio program, a compiled Windows runtime that reaches the
title, accepts menu input, saves to FlashRAM, relaunches into Goldwood gameplay
and exits cleanly, with zero unsupported device accesses. The v1.0 runtime also
passes the combined opening and Quarry timing validation recorded in the
[runtime fix log](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/development/boot-gameplay-fix.md#quarry-audio-delay-and-original-guest-timing).

Public CI exercises the build recipe, native output paths, launcher compilation
and source policy without a ROM. A successful build establishes compilation,
not original-console parity; a pristine Windows installation with installer
elevation and reboot still needs playtesting.
