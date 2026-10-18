# Renderer draw-call cache keys

The pinned RT64 DrawCallKey has 84 bytes of members and four bytes of tail padding on the supported 64-bit ABI. Its constructor path assigns the members but leaves the padding indeterminate. Hashing all 88 bytes can give different hashes for identical logical keys, growing the material lookup cache. Equality also returned raw memcmp, which inverted equality and included that padding.

A hash-guarded build-tree patch hashes and compares only the member range, with compile-time offset checks. Equality returns true for matching fields. An empty material-preset library returns no matches immediately rather than retaining negative cache entries. The pinned dependency checkout is unchanged; LF and CRLF copies of the same reviewed input are supported.

The regression varies every padding byte and every member byte. Against the pinned original it reports 88 failures; the corrected implementation reports zero. It also checks that an empty library retains no lookup keys and that a nonempty library keeps one cache entry for 256 padding variants of the same draw call. Renderer interpolation tests pass.

External thread sampling of the unchanged executable observed hash-table growth during the repeated late renderer hitch. Earlier timer instrumentation changed stack layout and made the hitch disappear, so those captures alone could not establish a remedy. A clean rebuild restored the hitch. The deterministic-key/empty-library correction then passes the unprofiled native route, Original and recorded-state checks. The sampling map type alone did not distinguish shader versus draw-call caches; the padding fixture and corrected replay provide the additional evidence.

This changes renderer bookkeeping, not gameplay physics, AI, game timing or textures. Native runs still follow wall-clock timing and can diverge. Loading/restart pauses and full owner-route visual/listening acceptance remain open. See [the issue record](issues/renderer-cache-key-instability.md) and [qualification](../../evidence/high-refresh-draw-key.json).
