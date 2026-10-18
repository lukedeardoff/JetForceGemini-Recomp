# Native audio playback and deforming bridge presentation

Normal Play uses a native wall clock. Original audio synthesis was scheduled
from delivered video-retrace notifications; missed/coalesced retraces could
reduce production, and already-built audio tasks waited for the next notification
gate. Catch-up attempts also exposed discarded PCM when the physical two-slot
AI FIFO rejected a submission. Completed PCM waited for a later host service
point; one observed buffer was ready about 108 ms before that service.

Native playback now replenishes an 85 ms host queue watermark through the
original audio manager, runs a verified ready audio task when the RSP is free,
and retains up to four AI buffers. Successful buffer submission immediately
copies PCM to the host device while preserving the guest floating-point state.
Sample generation and frequency remain the original ones. The policy allows at
most one notification per delivered retrace, refuses pending/full queues, and
leaves the separate loading schedule in control of its notification cadence.

The AI model defaults to the physical two slots; original instruction-clock
reference probes retain that behavior. Normal Play enables the native policy.
JFG_DIAGNOSTIC_NATIVE_AUDIO_PLAYBACK=0 selects the older path for private
comparisons. These are developer diagnostics, not additional video options.
Queue bounds, sample durations, interrupts, unsigned deadline overflow, countdown
byte neighbors and notification/format boundaries have regression coverage.

The bridge has a separate rendering correction. Behavior 55 rewrites local
vertex heights, so rigid-transform matching alone missed its bending. Logical
vertices now retain actor lifetime/model ownership and interpolate their world
positions. Bounded owned-camera validation handles near-plane motion without
mistaking it for a geometry cut; actual cuts, lifetime changes and ambiguous
ownership still hold history conservatively. Character matching rules remain
unchanged. JFG_DIAGNOSTIC_BRIDGE_VERTICES=0 is a private comparison override.

The original bridge must settle before it can bounce again, as verified by the
owner. This change preserves that physics behavior.

The 12-stall audio fixture had zero queue underruns, rejected buffers or payload
mismatches with the combined candidate. The fixture and failed experiments were
removed from the normal build. A six-minute muted replay and the final 360 FPS
replay report their actual counters in the qualification receipt. The retained
17,000-retrace Original comparison preserves RAM/save, instruction records and
rendering hashes. Hidden/muted captures do not establish audible or monitor
scanout acceptance. The bridge capture uses a test-warped route and does not
establish every other object or shadow is smooth.

See [qualification](../../evidence/high-refresh-audio-bridge.json),
[AUDIO-002](issues/walking-audio-interruptions.md), and
[RENDER-006](issues/tutorial-late-object-jitter.md). Detailed profiling is still
opt-in. The broader no-20-FPS objective remains open.
