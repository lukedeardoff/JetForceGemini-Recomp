# RENDER-005: Pickup particles desynchronize from interpolated gems

Status: **correction bounded-tested and delivered locally; owner live verification pending** (2026-10-13 UTC).
Severity: visible effect coherence defect. The owner confirms gems and drones
look good in visibility preview implementation `c9189c1`, runtime
`cb3de8f6593fa2abb0a12e7a2ea7f98ecf23ea817ad247505c1338a39dcd2292`, but particles
above gems appear to move out of sync with the gems.

Reproduction: observe gem sparkles while moving the camera and walking in the
Goldwood pickup area. Exact particle type, lifetime path and phase mismatch
remain unverified. Hypothesis: particle/effect transforms lack the actor-owned
billboard identity now used by gems, or follow a different presentation phase.
Do not attach unrelated particles to a pickup solely by proximity.

The owner approved one additional guarded run within the existing 10:57 UTC
deadline. Combat source cadence has priority. Muted hidden copied-save tests and
large evidence stay on D:. Preserve the same investigation/root/guard counters.
No remedy, test pass or delivered resolution is yet claimed. Related:
[RENDER-004](actor-jitter-edge-culling.md).


Source observation: the supported US billboard particle draw pass calls
camDoSprite with a 0x78-byte particle slot, not an actor-table object. Its caller
returns to 0x8005D59C. The previous actor identity lookup therefore rejects it.
Candidate tracks type-2 particle allocation (0x80060EC4), free (0x800610CC) and
library teardown (0x8005CE60), with a fresh monotonic lifetime for slot reuse.
The draw captures its own asset and world anchor, feeding the existing camera
and billboard interpolation; no nearest-gem association or simulation writes.
Bounded anchors expire after six VIs, asset changes hold endpoints, and level
changes clear all ownership. Other particle draw types remain outside this fix.


Initial lifetime-only captures at scene 5001 (new sparkle) and scene 5025
(overlap) are pixel-identical to the old preview despite additional validated
particle owners. They do not prove a visible correction. The old match key also
contains every local quad coordinate: an animated size/shape can reject the
same living particle. A refined candidate keys known billboards by lifetime,
asset, camera, vertex count and triangle connectivity; local geometry is no
longer their identity. Unknown geometry, new lifetimes, discontinuities and
topology changes retain conservative endpoint behavior. Local quad/texture
animation is still authored-rate; this correction targets world-anchor motion.


## Final candidate validation

Runtime `14746717614559b93fa535f9caf0a02fce50f23d2df3ab23aa6e9302607f30aa`. Nine focused suites pass, including immediate
free/reallocation, independent identical assets, expiry, teardown, malformed
ownership, animated quad dimensions, changed topology and new lifetimes.
The scene-5007 changing-particle pair has intermediate pixel differences: True;
source endpoint equality: True. All non-framebuffer guest bytes
match in the paired Custom replay. Final Original guest/renderer/save state
matches the previous preview exactly. The two earlier zero-difference pairs are
retained as limited evidence rather than credited as visible fixes.

The fix preserves identity through the effect's lifetime and uses topology,
asset and camera as compatibility checks. Local quad/texture animation remains
at its authored rate. Spawn/death, camera cuts, other particle types and full-game
coverage remain limited. Owner live verification of the reported desynchronizing
sparkles is still required; this is not a released resolution. Evidence:
[combat and particle qualification](../../../evidence/combat-particles.json).


## Delivery held for performance verification

The implementation is ready for review, but the desktop shortcut and installed
visibility preview remain unchanged. Final combat has real drops and 8/62 audio
underruns at 120/360; late concurrent-runtime load prevents clean attribution.
The post-pause trace has 988 samples, no repeats/rewinds, and three skipped phase
steps. Functional/state/pixel verification does not establish smooth performance.
Do not replace the working preview or call combat fixed until a controlled
single-runtime comparison resolves this. Both candidate sources and all evidence
are preserved. The next bounded run should first coordinate exclusive runtime
testing, compare unchanged/candidate builds on the same copied checkpoint, then
profile remaining host execution/renderer costs if genuine drops persist.

Implementation commit: 83cd8af0bad951d27939adadb74f374840d1303d. The committed build retains
qualified runtime 14746717614559b93fa535f9caf0a02fce50f23d2df3ab23aa6e9302607f30aa.
A copy is retained in the private D: evidence directory. Desktop delivery is
held pending the controlled performance comparison above; no release was made.

## Delivery after controlled combat qualification

Particle implementation 83cd8af is retained in a93faa5ef61b86536327ffa83d48fc95835a21ff.
The new high-refresh-combat-spans-20261013 preview includes it. All three isolated
combat replays maintained near-30 scene cadence with zero audio underruns;
package files and launcher initialization pass. Owner live particle confirmation
remains pending. Historical held-delivery measurements above are preserved.
See [current qualification](../../../evidence/combat-native-spans.json).
