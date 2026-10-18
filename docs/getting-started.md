# Getting started

Play the Windows x64 v1.2.0 release using your own supported North American
Jet Force Gemini ROM. The release is prebuilt; no Git, Python, Visual Studio,
WSL, or first-run compilation is needed.

## Requirements

- Windows 10 or Windows 11, x64.
- A compatible hardware graphics device and its working driver. Software-only
  virtual machines cannot run the game.
- The supported 32 MiB US big-endian `.z64` ROM for the initial import. Other
  regions, modified ROMs, and byte-swapped files are not supported.
- A writable extraction folder and space for the 32 MiB imported ROM.

## Install and play

1. Download **JFG-1.2.0-windows-x64.zip** from
   [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0).
2. Extract the complete ZIP, keeping its program, libraries, and license files
   together. Open `JFG-Launcher.exe`.
3. Use **Select ROM…** to import your supported ROM. The launcher checks the
   entire file and stores a local copy in your profile's `roms` folder.
4. Click **Play**. You may move or delete the original ROM after import.

The central ROM card disappears after import. Use **Game > Select ROM…** if
you need to select another file. The imported copy is still needed for play;
it supplies the game's assets and is never uploaded. No ROMs or game assets
are included in the release.

## Update from v1.0 or the installer beta

Extract v1.2.0 into a new folder and open its launcher. Your existing profile,
saves, and settings are reused. A valid previously selected ROM is imported
automatically; if its original file is missing, use **Select ROM…** again.
Old build tools are no longer prerequisites for the released package.

F11 toggles fullscreen. The game remains centered in a 4:3 area, and cutscenes
retain their original letterbox bars. See the [launcher guide](development/launcher.md)
and [changelog](../CHANGELOG.md).

## Build the latest source

First install the prerequisites in [ROM build setup](development/rom-bootstrap.md).
Then run these commands in PowerShell:

```powershell
git clone https://github.com/TK22-26/JetForceGemini-Recomp.git
cd JetForceGemini-Recomp
python scripts/build_from_rom.py --rom 'D:\Games\my-copy.z64' --play
```

Replace the example ROM path with your own. This command downloads pinned
source dependencies and builds the game; install the developer tools first.
Record `git rev-parse HEAD` if you report a source-build problem.

The released launcher uses the runtime bundled beside it. Maintainers can
prepare a matching package from a reviewed source build using
[the release instructions](development/releases.md).

## Start playing

Try movement, jumping, aiming, shooting, and opening and closing pause.
At a normal save opportunity, check that quitting and relaunching restores
your progress. Continue playing normally; these checks do not require a report
unless something goes wrong.

For a problem, follow [playtesting and reporting](playtesting.md).
For code changes, see [contributing](../CONTRIBUTING.md).
