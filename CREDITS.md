# Community credits

Git history retains authorship of contributed code. This list also acknowledges
reports and investigations that informed independently implemented fixes.
Dependency attribution remains in [third-party notices](THIRD_PARTY_NOTICES.md).

## Luke Deardoff (@lukedeardoff)

- Reported and investigated the opening asteroid texture corruption, including
  the stale texture-offset addresses reproduced in [PR #9](https://github.com/TK22-26/JetForceGemini-Recomp/pull/9).
  His trace identified the incorrect texture addresses and guided the correction.
  See [findings](docs/development/boot-gameplay-fix.md#opening-asteroid-textures).
- Reported the fresh-build recovery failure and proposed allowing external build
  caches while retaining restrictions inside the checkout, in his
  [community branch](https://github.com/lukedeardoff/JetForceGemini-Recomp/tree/c8253db8b70c7e2fd166bc9efcd7d101f2d3c819).
  The recovery helper incorporates that output-policy correction with regression
  coverage for external caches, existing files, and checkout boundaries.
- Shared a lens-flare depth-check diagnostic in his community branch. An
  adaptation helped identify missing CPU-visible depth data in the opening
  cinematic; the effective-RDP-state correction was implemented independently.
  See [depth findings](docs/development/boot-gameplay-fix.md#opening-lens-flare-occlusion).
