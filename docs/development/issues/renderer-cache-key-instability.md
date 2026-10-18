# PERF-003: Renderer draw-call key instability and late hitch

Severity: high. Status: verified in regression/reference tests and bounded native replays; delivered locally; owner full-route acceptance pending (2026-10-16 UTC).

Symptoms: a repeated late Goldwood update at input poll 7620 takes about 65-67 ms. Graphics submission accounts for 39-42 ms, with only 0.37-0.46 ms interpolation preparation and under 1 ms in the six measured renderer waits. The event has no ROM reads or cold overlay loads and is separate from the earlier combat asset-loading hitch.

Affected local native-ROM preview runtime: 5f902049bd3e4a991a020f17e63a51d0291f28709057a9aa7efc03097692764b. The same source rebuilt without private timers reproduces the event. Two instrumented builds do not, making observer effects a material limitation. An external sampler reproduces the hitch and finds unsigned-64/bool map growth; that type is shared by two RT64 caches, so sampling alone is not proof of which cache.

Cause and remedy: [draw-call key implementation](../renderer-draw-call-keys.md). Indeterminate tail bytes participate in hashing, and equality is inverted. An independently failing regression establishes both defects. Stable member-only keys and an empty-library fast path avoid spurious cache work; all original material matching still applies when presets exist.

Validation: 88 original regression failures become zero; renderer interpolation tests pass. The 17,000-VI Original comparison preserves RAM/save/instructions/probe/renderer hashes. The 4,000-VI recorded-clock comparison preserves RAM/save/instructions/interpolation and original ROM-transfer policy. Final default 360 FPS combat has no missed two-VI intervals. The unprofiled 120 FPS late-route replay does not repeat the poll-7620 hitch. Both native checks have zero audio underruns and unsupported accesses. Native route differences and hidden-window observation limit the claim; this does not prove universal no-20-FPS behavior.

Remaining native gaps are recorded in the qualification receipt; restart/cold loading are still open. Original bridge settling physics and native shooting assistance remain unchanged. Listening/bridge presentation still require owner review.

Evidence retained privately under route-audio-bridge-20261016: late-renderer-attribution, late-renderer-cross-capture-review, clean-renderer-rebuild, external-renderer-sampling-review, sampled-hitch-time-correlation, draw-key-fix, draw-key-native-late-v2, draw-key-original-reference, draw-key-recorded-reference and draw-key-default360. Diagnostic failures remain in PERF-002: missing DLLs, extra stderr rejected by the parent, and an invalid profiler-disable configuration. None count as clean acceptance.

Transferable lesson: hash logical fields rather than object padding, make equality agree with hashing, and do not retain negative-cache entries for an empty configuration. Instrumentation can alter uninitialized stack bytes and hide a defect; retain an unchanged executable control and use external sampling when necessary.


Local delivery: both High Refresh Preview and Jitter Capture desktop links select source 27ddef0a5ed4afd1f1f9c26a5590b590cda5fe8e, runtime SHA-256 db65b460f399b550ca8a3ad0d98f5374d5e00a95dc4a7073edfa2649fa6772c6. Manifest and launcher/profile checks pass; all 40 checked profile files and shortcut arguments are preserved. Includes native audio, bending bridge interpolation and ROM loading improvements. Owner live acceptance and broader restart/load gaps remain open. [Current qualification](../../../evidence/high-refresh-draw-key.json).

Presentation limit: the default-360 combat sections averaged about 353-359 successful present returns per second, with a median interval near 2.776 ms and occasional longer presentation intervals. Stable two-VI game updates do not prove perfect output pacing or monitor scanout. This is distinct from the removed late game-update hitch.
