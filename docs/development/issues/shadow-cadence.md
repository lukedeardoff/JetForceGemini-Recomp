# RENDER-007: Ground-shadow cadence at high refresh

Status: owner-reported sluggish shadow motion; source-level interpolation gap
identified, visible lag magnitude and correction not yet verified.
Severity: medium visual smoothness limitation; separate from combat FPS loss.

The owner noticed Juno's ground shadow looking less smooth than the character
in the expanded tutorial capture, while acknowledging uncertainty. Affected
preview is the develop-merged high-refresh runtime used by capture
20261013-172104. That recording has timings and a final image, not a continuous
video of the shadow; it cannot establish the shadow's exact visual delay.

The renderer interpolates owned character vertices and camera/model transforms.
Small intensity shadow-mask passes are excluded from scene interpolation, and
the original game reads them back for CPU blur/attenuation before sampling them.
Those textures and projected texture coordinates do not currently receive
caster-owned interpolation at each output phase. Thus 120 OUTPUT does not mean
120 newly animated shadow masks. A moving character can have smoother animation
than its shadow even with steady output. Texture and mask cadence is distinct
from a demonstrated temporal lag in the reported scene.

Source: rt64_f3ddkr.cpp semantic-target selection/prepare_interpolation,
rt64_interpolation_adapter.hpp apply_presentation_interpolation, and
rt64_shell.cpp CPU mask writeback. The pinned decomp shadowGenerate and shadow
projection routines establish the CPU-authored receiver/mask path. No mask
readback or CPU processing was removed to make a performance counter rise.

Next verification needs a fixed-route sequence of actual presented frames with
visible feet and shadow, compared at equal camera/body phase. A remedy should
bind the projected shadow to its caster/receiver and preserve original lighting,
CPU mask edits and clipping. Simply shifting receiver geometry or crossfading
textures can introduce floating shadows or ghosting and is not an accepted fix.

Related: [late tutorial jitter](tutorial-late-object-jitter.md),
[combat cadence](goldwood-combat-slowdown.md), and
[seams/lighting](character-seams-lighting.md). No released resolution is claimed.

## Bounded frame review (2026-10-13)

The final Original/Custom comparison preserves every non-framebuffer guest byte
and the save. The custom eight-frame capture shows Juno beside a wall, without
a clearly measurable ground shadow, so it cannot establish the reported lag or
validate a remedy. Readback probes have 2/3 audio underruns, unlike zero in the
separate native Play tests; do not use them as performance acceptance.

RT64 already supports texture-coordinate velocity and generic tile interpolation.
The missing part identified here is explicit caster/receiver ownership and
phase handling in this game's shadow path, not a claim that RT64 lacks UV
interpolation. No shadow correction is included in this review. Keep RENDER-007
open and require visible shadow/body comparisons before declaring a fix.

## Caster/receiver probe (2026-10-13 continuation)

A muted moving-Juno sequence provides a visible ground shadow. A bounded private
probe observes 90 projection calls and 800 generated vertices. Juno's projected
footprint changes between 8, 12 and 16 vertices as it clips across floor triangles;
all receiver pointers in this sample are static terrain. The shadow texture is
separately generated, then CPU-processed. Projector UVs depend on caster yaw,
position and height, while receiver vertices are quantized world coordinates.

The suspected offscreen presentation wait is ruled out in the measured combat
sample: 102 passes total about 0.017 ms of that wait, with a zero median. This does
not rule out mask rendering/readback cost. No shadow interpolation remedy has
been qualified. A geometry-ordinal match or unconstrained footprint translation
would not preserve clipping and is not included. Private evidence:
route-fix-20261013/shadow-named-review.json and graphics-wait-review.json.

## Continuation disposition, 2026-10-13

No shadow remedy is included. The visible moving-shadow probe and source audit
establish changing clipped receiver topology (8/12/16 vertices) and a separately
CPU-processed mask. They do not quantify the owner's perceived lag or validate a
caster/receiver interpolation scheme. Offscreen present-queue wait is negligible
in the measured sample; GPU/readback cost is a separate measurement. A future
change needs stable caster/receiver/clip identity and mask/UV phase coverage.
Ordinal matching or translating the complete footprint is not a qualified fix.

## Continuation review, 2026-10-14 UTC

The preserved moving-shadow image sequence was inspected again. A bounded
fixed-region dark-pixel centroid analysis is exploratory only: the region
can include terrain and becomes invalid as the camera or occluders move.
It cannot establish a shadow lag measurement. The clipped receiver polygon
and CPU mask remain distinct from Juno's interpolated skeleton. Translating
the current footprint or matching clipped vertex ordinals would be unsafe
across changing terrain/topology. No qualified shadow correction is selected.


A further pinned-source audit rules out scratch polygon offset +0xA as a stable
receiver-triangle identity. Original instructions at 0x8001E99C/0x8001E9A4 derive
that field from (batchFlags >> 22) & 7, stored at 0x8001ED5C. It is not a unique
triangle key. Clipped intersection vertices also discard original positive
vertex identifiers. This prevents using either the scratch ordinal or that
material field as an interpolation identity. No shadow correction is selected;
receiver/clip provenance and separately CPU-processed mask phase remain required.
Source: pinned shadow clipping/projection routines; filename labels can differ
from actual instruction PCs, so use the instructions themselves for hooks.
