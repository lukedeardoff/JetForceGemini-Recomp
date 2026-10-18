# CPU-visible framebuffer writeback

The recorded diagnostic found that rendered results in RT64's private RAM
were not being transferred back to guest RAM for a CPU depth-buffer read.
Actor hashes alone had not excluded this mismatch.

`JFG_PHASE9_RENDERER_WRITEBACK_PROBE=1` enables the diagnostic ownership bridge:

- Complete synchronous rendering and identify the framebuffer ranges written.
- Validate owned CPU bytes against the submitted snapshot before committing.
  A conflicting CPU write rejects the entire operation.
- Commit owned ranges before SP/DP notifications; preserve unrelated bytes and
  handle word-swapped RAM and differing guest/backing sizes.
- Disable render skipping, which cannot preserve CPU-visible GPU output.

The experiment used conservative framebuffer ownership. Multiple queued
snapshots, overlapping ownership, and a complete asynchronous coherence model
remain separate coverage. Host rendering duration does not determine guest time.

The bounded branch proof did not fix the later pacing mismatch. See the
[runtime architecture](../development/architecture.md#boot-os-and-timing) for the recorded frontier
and [Git history](../README.md#historical-material) for full experiment evidence.
