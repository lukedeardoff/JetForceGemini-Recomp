# Cosmetic enhancements (unofficial, community)

Work by Luke Deardoff ([@lukedeardoff](https://github.com/lukedeardoff)), made with AI assistance (Claude).
This is an **enhancement** branch, not a compatibility fix, and it is not an upstream contribution.
Everything here is opt-in, and no ROM data, game assets or generated game code are included.
You still build the game yourself from your own ROM with the normal launcher/build flow.

Tested on Windows 10 22H2, RTX 2070 Super (RT64 on D3D12), launcher source `c88c411` (v0.4.0-preview.1).

## What it changes

| Area | Problem in the PC build | Change |
| --- | --- | --- |
| Title/menu vignette | The 80x60 I4 vignette mask is upscaled into hard stair-steps | HD replacement texture pack (`community/texture-packs/jfg-vignette`), auto-loaded at startup |
| Actor shadows | With `JFG_PHASE9_RENDERER_WRITEBACK_PROBE=1` the 64x64 shadow buffers are written back and re-read at native resolution, so shadows become large blocky squares | Writeback skips framebuffers narrower than 128 px (the flare depth check still works) |
| Actor shadows | Without the above, shadows are HD but hard/pointy instead of soft like on hardware | Optional RT64 compute pass that box-averages the shadow at native 64x64 resolution and reconstructs it with a Gaussian (`JFG_SHADOW_BLUR`) |
| RT64 inspector | Pausing (F4) during gameplay hit the fail-closed `auxiliary-target` trap | Paused submissions are reported as paused instead of as a renderer failure |
| Crash reports | Native crash reports only gave an exe-relative RVA | Also prints `jfg-crash-site` with module+offset for every stack frame |
| Developer inspector | Always off | `JFG_RT64_DEVELOPER=1` enables the RT64 inspector (F1) |

## Files

- `src/renderer/rt64_shell.cpp`, `include/jfg/renderer/rt64_shell.hpp`: texture-pack auto-load, writeback width filter, pause handling, `JFG_SHADOW_BLUR`.
- `src/boot/native_boot.cpp`: `JFG_RT64_DEVELOPER`, paused-renderer check, crash-site report.
- `patches/rt64/jfg-shadow-soften.patch`: RT64 changes (new `FbShadowSoftenCS.hlsl` compute shader, pipeline creation with a validity check and fallback, per-dispatch pipeline selection). The upstream `FbReinterpretCS` shader and all shared headers/struct layouts are unchanged. If the driver rejects the new pipeline, the game logs it and keeps standard shadows.
- `community/texture-packs/jfg-vignette/`: the vignette texture pack (original artwork by Luke; RT64 hash `f35c64d7a44bc219`).

## How to use

1. Check out this branch and build normally once so `tools/upstream/rt64` exists.
2. Apply the RT64 patch:
   `git -C tools/upstream/rt64 apply ..\..\..\patches\rt64\jfg-shadow-soften.patch`
3. Rebuild: `python scripts\build_from_rom.py --rom <your rom> --resume-native <your build folder>`
4. Copy `community\texture-packs\jfg-vignette` into `%LOCALAPPDATA%\JFGRecomp\texture-packs\` (any subfolder with an `rt64.json` there is loaded at startup).
5. Launch with these environment variables:
   - `JFG_PHASE9_RENDERER_WRITEBACK_PROBE=1` (lens flares)
   - `JFG_SHADOW_BLUR=1.5` (shadow softness in native texels; `0` = off, higher = softer)
   - optional `JFG_RT64_DEVELOPER=1` (F1 inspector)

On startup the console prints `[JFG] shadow softening pipeline ready.` (or a fallback message).

## Notes for anyone editing RT64 here

- The build does not rebuild RT64 objects when a shared RT64 header changes. Changing a struct layout without forcing a full rebuild causes hard-to-diagnose crashes.
- plume's D3D12 backend does not check `CreateComputePipelineState`. A rejected shader becomes a null PSO and crashes later in `SetPipelineState`. New shaders should be separate pipelines with a validity check.
