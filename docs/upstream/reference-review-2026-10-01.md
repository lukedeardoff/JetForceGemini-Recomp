# JFG upstream reference review

This review records useful information from the external JFG decompilation and
recompilation projects for future implementation and validation. The
[reference catalog](reference-catalog.json) contains 28 source-attributed
entries: 18 candidates for further work, three existing capabilities, three
reference observations, and four rejected approaches. Each entry identifies
our corresponding files and the evidence needed before changing behavior.

The strongest findings concern FlashRAM erase size, overlay DATA relocation
offsets, and overlay lifetime semantics. These are source observations and
test proposals. No runtime behavior, dependency pin, or gameplay acceptance
changed as part of this review. A later pause-screen repair used JFG-UP-014
to distinguish guest suspension from cold loading.

## Sources and revisions

The review retrieved full repository history and inspected the latest pushed
branches available on 2026-10-01, including an open menu pull request.

| Source | Reviewed revision | Status |
| --- | --- | --- |
| [JFG decomp master](https://github.com/Ryan-Myers/Jet-Force-Gemini/tree/efd5abb1c79636e297b831f7c2d5bf47eac39c0c) | `efd5abb1c796` | Merged default branch |
| [JFG decomp OverlayHandling](https://github.com/Ryan-Myers/Jet-Force-Gemini/tree/948e1f5acaa02d2df359ce647ca1f439036975d1) | `948e1f5acaa0` | Newer unmerged branch |
| [JFG decomp menu PR 37](https://github.com/Ryan-Myers/Jet-Force-Gemini/pull/37) | `d45123d1c528` | Open PR; US check passed, kiosk check failed |
| [JFG Recomp main](https://github.com/Dubstroy/JFG-Recomp/tree/61c62e74e6b587341e54433bfe137bb7ca1c5558) | `61c62e74e6b5` | Latest default branch |

Our decomp dependency remains at `b49aa4791e8fb1e7acd3bba10346876358d0a9a7`.
The reviewed master contains 35 additional commits, and OverlayHandling adds
another 11. Some useful findings already existed at our pin; newly documenting
them does not mean they are new upstream implementations.

## Work to prioritize

The catalog supplies immutable file and line citations for each item below.

1. **FlashRAM sector erase, JFG-UP-020.** The decomp describes 128 pages of
   128 bytes per sector. Our sector-erase dispatch reaches a page-erase path
   that clears 128 bytes. Validate the device contract with three distinct
   sectors, erase from a page inside the middle sector, and check the entire
   16 KiB sector and unchanged neighbors. The effect on current gameplay has
   not been measured; previous persistence checks do not resolve this case.
2. **Overlay DATA patch sites, JFG-UP-012.** Upstream applies relocation type 3
   relative to the data base. Our transformer adds the text extent for the
   main module but leaves the overlay offset unchanged. A synthetic overlay
   whose DATA relocation starts at zero can test the expected normalized
   offset. Occurrence in the supported ROM remains unverified.
3. **Overlay lifetime, JFG-UP-009 and 014 through 018.** The countdown named
   `refCount` and the low-memory zero-count sweep are separate rules; header
   prose incorrectly combines them. Suspend/resume preserves data and BSS
   while replacing text, whereas our host cold-reload operation resets state.
   Generated guest code may already supply the preserve-state operation.
   Callback ordering, retained relocation tails, and delayed allocator frees
   need focused observations before adding host behavior.
4. **Dependency update prerequisites, JFG-UP-013 and 027.** New zero-based
   overlay addresses, alignment, secondary relocation sections, symbol
   handling, and SDK names require section-qualified identity and regenerated
   inventory checks. Updating a dependency pin without these checks would
   invalidate assumptions in the current build pipeline.

## Additional useful knowledge

The save-format and controller entries, JFG-UP-021 through 023, separate
successful device persistence from a valid game save. They record checksums,
slot validity, protected pages, reset behavior, and the SI transaction ordering
that controls pending writes, pad samples, rumble, and CIC exchanges. An empty
SI poll can retain old samples and pending flags; consuming a request flag is
not proof that its save operation succeeded.

The audio entry, JFG-UP-024, identifies useful boundary cases: callbacks exactly
at a frame end, parameter alignment to 16 samples, equal-time FIFO ordering,
empty clients, command chunking, and queue saturation. Our host audio bridge
tests do not establish these guest scheduling rules.

Camera and squad entries, JFG-UP-025 and 026, propose readable metadata for
state already included in opaque comparisons. Squad membership or tribal
classification alone does not prove a rescue. The menu PR adds useful names
for settings and mappings, but its unmerged status and failed kiosk check
limit what can be inferred. Controller clamping already exists at our pinned
revision; applying it again in the host would alter input.

Existing relocation parsing, addend handling, HI16/LO16 support, and backpatch
work remain useful project capabilities. Upstream helper scripts contain
indexing and annotation inconsistencies; use the guest loader as evidence
before trusting a helper's labels.

## Recomp project findings and progress comparison

The other recomp's reachable history pins the stock N64Recomp revision used by
our project. Its advertised custom JFG relocation setting has no corresponding
published implementation in that history. Its ROM scanner offers ideas for
candidate ranking and bounded decompression diagnostics, but its boot control
flow and mapping assumptions should not be imported as verified behavior.

| Measurement | Our retained build inventory | Other recomp's committed counters |
| --- | --- | --- |
| Required generated CPU bodies | 2,909 of 2,909 | No comparable published inventory |
| Aliases and alternate entries | 824 aliases; 30 entry helpers | Counting policy unavailable |
| Reported analyzed entries | No equivalent analysis counter | 1,141 of 11,253, or 10.14% |
| Reported successful entries | Generation and native compilation verified locally | 1,140 of 11,253, or 10.13% |

Our generation coverage is 100% for that inventory, with zero game-function
stubs. This does not measure semantic correctness, unimplemented runtime
dispositions, or campaign completion. The other project's stats producer and
per-entry inventory are unpublished; its updater can preserve old counts while
refreshing the displayed date. A strict shared percentage needs the same
ROM/ELF, tool revision, executable-section filter, and alias policy. No overall
completion percentage follows from these data.

The reviewed scheduler, video, audio-manager, and opaque campaign material
does not establish a fix for the existing selected-state mismatch at update
1909. This reference work does not reopen that investigation or reset any
guard allowance.

## Attribution and integration boundary

The JFG decomp has no observed project-level root license. Dubstroy's recomp
has an MIT notice with a placeholder copyright holder, and its tree contains
boot binaries and raw reports that are unsuitable for this source handoff.
Full checkouts and review evidence remain in ignored private storage.

Only paraphrased observations, source links, and proposed validation work are
added here. No upstream implementation, header, ROM body, asset, generated game
code, or raw symbol export is imported. Existing third-party notices and the
project license continue to apply; this document grants no additional rights.

Integration validates catalog structure, cited commit/path/line existence,
local reference paths, public-source hygiene, preservation of the guard
instructions, and whitespace. Those checks establish reference integrity,
not runtime correctness or copyright clearance.


## Later focused reviews

- [VibinToast JFG source and v1.1.3 review, October 15](reference-review-2026-10-15-vibintoast.md): separate pinned evidence for the owner-requested combat/high-refresh comparison; does not change this review's historical baseline.
