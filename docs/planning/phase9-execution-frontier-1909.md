# Recorded selected-state boundary: update 1909

This September 2026 record applies to its pinned experiment, not to the
current launcher. See [project status](../status.md) and
[runtime architecture](../development/architecture.md#boot-os-and-timing).

## Result (2026-09-26)

The opt-in original-OS path matched **1,908 consecutive completed-update
states** against the corrected-Mupen reference. The first selected-state
mismatch is update **1909**, where the camera and 18 actor records differ; two
independent native/reference capture pairs reproduce it. No runtime code was
changed for this extension; the executable is the one that passed the
[1300 prefix](https://github.com/TK22-26/JetForceGemini-Recomp/blob/2c55b0f082395e2be4407958637dbb4effb3c43d/docs/planning/phase9-execution-frontier-1300.md).

| Completed update | Native consumed VI | Reference consumed VI | Polls (both) | Elapsed-VI word (native / reference) |
| --- | ---: | ---: | ---: | --- |
| 1907 | 4589 | 4589 | 1917 | 3 / 3 |
| 1908 | 4592 | 4593 | 1918 | 3 / 4 |
| 1909 | 4595 | 4596 | 1919 | 3 / 3 |
| 1910 | 4598 | 4599 | 1920 | 3 / 3 |

The elapsed-VI word is `0x800a3374`; the pacing-mode byte at `0x800fecac`
stays 3 on both sides. Focused RDRAM shows no actor differences at 1908 and
18 differing actor ranges at 1909, with the same neutral input poll between
them; all 2,290 shared polls agree. The timing difference precedes the
actor/camera difference: a concrete lead, not proof of the causing CPU or
device operation.

This is a selected-state prefix, not full-RAM or complete runtime equality,
and the reference is a compatibility profile, not physical-N64 timing. Do not
insert a VI, force the elapsed word, alter the recorded input or shift the
comparison.

## Follow-up observations

- A [pacing receive observation](https://github.com/TK22-26/JetForceGemini-Recomp/blob/2c55b0f082395e2be4407958637dbb4effb3c43d/docs/planning/phase9-pacing-receive-observation.md)
  found the producer's queue already holding 2 native / 3 reference messages
  at invocation 1908, so the lead is earlier message availability, not the
  producer's counting logic.
- A [device-event observation](https://github.com/TK22-26/JetForceGemini-Recomp/blob/2c55b0f082395e2be4407958637dbb4effb3c43d/docs/planning/phase9-device-event-observation.md)
  added bounded raw engine events with unchanged traces and focused RDRAM; it
  again passes through 1908 without explaining 1909.
- `scripts.phase95_native_replay` accepts `--execution-profile original-os-probe`
  or `cooperative`, which discard inherited `JFG_PHASE*` variables and build
  only their declared flags; omitting the option is labeled
  `legacy-environment`. The [profile handoff](https://github.com/TK22-26/JetForceGemini-Recomp/blob/2c55b0f082395e2be4407958637dbb4effb3c43d/docs/planning/autonomy-execution-profile-handoff.md)
  carries execution and runtime pins through update, determinism and
  candidate-retest packets.

Next: trace execution and device completion and guest queues around update
1908 to separate CPU-work accounting from device-event timing. Further work is
admitted only by the [production guard](autonomy-progress-guard.md).

## Evidence

Private directories under `tools/private/`: `execution-prefix-native-20260926a`
and `execution-prefix-oracle-20260926a` (6,000-VI runs; `comparison-1908.json`
passes, `comparison-2278.json` first differs at 1909), and
`execution-prefix-focus-native-20260926a` / `execution-prefix-focus-oracle-20260926a`
(4,800-VI runs covering updates 1907-1910, whose record sequences equal the
longer runs' prefixes). The generated root, input export, saves, US ROM and
corrected reference are those pinned by the 1300 evidence; the longer run
consumed 2,299 of 10,881 route polls.

| Artifact | SHA-256 |
|---|---|
| Native executable | `6edd2c402141dce44a2a0d4f81f4bdf66e9eedefcf0f10d36133c5b726106397` |
| Native EXE/DLL runtime | `d45c88d2d798f20c9b7d071895e148805ae9d6d3bdcce48960e03de345a1ffd9` |
| Longer native update trace | `5c0b4169c38b727e1bf2178701ed2c06f62c1832b05904205522323aa16b936c` |
| Longer reference update trace | `08471f7456fba39103ac0036e8599b1f23b502e7549a20ff3ed7a47329dd0e47` |
| Focused native update trace | `45ee833c282ef24bd4e07f8377e0528a73aed58148b5f6d1059ea7edcac88f86` |
| Focused reference update trace | `daf32c850bb38e9b9306298c4a057c4457854a3ae1e95fc75ba77c7eaf90ecb8` |
| Focus comparison | `88e77ddb0e4b3dcfd97a20571c52806981727e8e6ef9cefdcea27f55cb71d4ce` |
