# Development setup

Use this guide to build the host shell and run source tests without a ROM.
For the playable game, use [ROM build setup](rom-bootstrap.md).
For playtesting, start with [getting started](../getting-started.md).

## Requirements

- Git and Python 3.11 or newer.
- CMake 3.20 or newer and a C++20 compiler.
- Visual Studio 2022 with C++ tools on Windows, or Ninja with GCC/Clang on Linux.

Keep dependencies under ignored `tools/` and game inputs outside Git.
Automated agents must run checks through the supervisor required by
[AGENTS.md](../../AGENTS.md); the commands below describe the human workflow.

## Python environment and validation

```sh
python -m venv tools/venv
```

Activate it with `tools\venv\Scripts\Activate.ps1` in PowerShell or
`source tools/venv/bin/activate` in a Linux shell. Then run:

```sh
python -m pip install --require-hashes -r requirements-dev.lock.txt
python scripts/validate_project.py
python -m unittest discover -s tests -p "test_*.py" -v
```

## Windows builds

The build command locates CMake and Visual Studio automatically. It uses a
short, separate output directory for each checkout, including deep source paths:

```powershell
python scripts/build_windows.py --config Debug --test
python scripts/build_windows.py --config Release --test
```

Output lives in `%LOCALAPPDATA%\JFG\b\<checkout-id>\c`; the command prints
the exact path. Use `--build-root D:/JFG-builds` to choose another short, writable
local cache location. Existing source, builds and dependency caches are preserved.

## Linux builds

```sh
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

These default targets build the host shell (`jfg`) and tests. The shell does
not run the game. The [ROM build recipe](rom-bootstrap.md) produces
`jfg-native-boot` for Windows playtesting.

## Choosing checks for a change

Run relevant focused tests while working and the applicable build/test suites
before submitting behavior changes. For documentation, inspect local links,
commands, and `git diff --check`. Run the repository hygiene check before pushing:

```sh
python scripts/check_repository_hygiene.py --history
```

[CI](https://github.com/TK22-26/JetForceGemini-Recomp/actions/workflows/ci.yml)
records the exact checks used for public pull requests. See
[test quarantine](test-quarantine.md) for the documented Linux test exclusion.

## RT64 shader dependencies

The pinned RT64 build tracked shader entry files but not their shared
includes. Following [Luke Deardoff's report in issue #4](https://github.com/TK22-26/JetForceGemini-Recomp/issues/4#issuecomment-5985126218),
the host build adds the shared `.h` and `.hlsli` dependency set to the existing
shader-generation commands, leaving the upstream checkout unchanged. A header
edit rebuilds shader outputs; unchanged builds stay incremental.

The `jfg.rt64_shader_dependencies` test checks shared, nested and newly added
headers with synthetic files across DXIL, SPIR-V, Metal and preprocessed
shader rules, without a ROM or shader compiler. MSBuild ignores Windows Temp
and AppData for dependency tracking, so the fixture runs in the checkout's
ignored `build/shader-tests` when the build cache lives there; a checkout
inside those directories needs an explicit `--work-root` elsewhere.
