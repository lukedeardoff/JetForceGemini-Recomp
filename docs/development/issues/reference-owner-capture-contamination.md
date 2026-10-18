# DIAG-002: Reference tutorial capture inherited gameplay overrides

- Severity: high for test validity; owner controls impaired.
- Status: reproduced setup defect; corrected capture script prepared and checked; further reference testing stopped at owner request.
- Reported and last updated (UTC): 2026-10-15.
- Affected profile: private owner capture of VibinToast v1.1.3, Original frame rate, Xbox controller.
- Related: [reference review](../../upstream/reference-review-2026-10-15-vibintoast.md), [combat timing](goldwood-combat-slowdown.md).

## Symptoms and reproduction

The owner reported difficult default controls, inability to aim/jump as expected, frequent frame drops and missed inputs, and unexpectedly strong bullet targeting. Launching the owner reference session from the inherited automation script reproduced a classic-controller configuration with scripted gameplay overrides. The owner reached tutorial levels 21, 16, 26 and 157. The initial waiting period is excluded from the timing analysis.

## Evidence and cause

The owner capture script retained JFG_DEBUG_CHEATS=1 and JFG_DEBUG_HOLD_WEAPON=0 from earlier automated route experiments. Source inspection confirms that the first flag enables invulnerability, unlimited ammunition, all weapons and all levels. The second repeatedly restores weapon slot zero, potentially overriding weapon-selection inputs. Neither is a logging-only option. The options file omitted dual_stick, whose upstream default is false. The controller audit reports dual stick inactive throughout the recorded run, including after the instructions to enable it; no successful setting change is recorded.

These are assistant capture-setup errors, not evidence of upstream default cheats or defective modern controller bindings. They were disclosed to the owner after source audit. Prior descriptions of this session as an ordinary/default gameplay comparison are withdrawn. The frame stalls remain measured observations under this contaminated profile. Their existence does not prove all missed inputs were caused by stalls.

Private evidence: vibintoast-owner-tutorial-20261015, its analysis/timing-review.json, original immutable request 479, and clean-capture-preparation.json. The original save/profile and installed preview were not modified.

## Attempts and results

- Initial capture: normal menu/level entry and muted sound, but inherited debug overrides and classic controls made it unsuitable as a default-game comparison.
- Audit: confirmed actual flags against source, controller state against runtime logs, and saved Original rate. No auto-aim cheat was identified among the enabled overrides.
- Correction: prepared a separate capture script that removes cheat, forced-weapon and scripted-input environment variables; clears inherited JFG/RT64 variables before adding only logging options; explicitly enables the built-in Xbox dual-stick layout with normal triggers. It retains Original frame rate and mute.
- Validation: prepared Python compiles and configuration assertions pass. No replacement game was launched and no clean owner route is claimed.

## Timing observations and remaining limits

| Level | Updates | Measured mean updates/s | p99 update interval | Intervals over 50 ms |
|---|---:|---:|---:|---:|
| 16 | 2880 | 30.00 | 34.7 ms | 0 |
| 21 | 5850 | 28.07 | 100.2 ms | 224 |
| 26 | 405 | 22.52 | 100.5 ms | 83 |
| 157 | 39 | 29.99 | 34.5 ms | 0 |

The analysis begins at first observed player displacement, removes the final two seconds near close, and selects gameplay mode. Per-update durations are rounded to 0.1 ms. Thirty-update diagnostic windows reach 15.0 updates/s. Level summaries include transitions and cannot isolate combat CPU cost. The log's work field includes waits; it is not CPU-only time. Raw host-controller edges and per-present timestamps were not captured, so missed presses and output frame pacing cannot be reconstructed exactly.

## Resolution and transferable lessons

The setup correction is prepared, not live-verified or released. Any new capture must retain an explicit environment/configuration receipt and verify the active controller mode before user handoff. Keep debug gameplay overrides separate from logging. A zero-valued environment variable can still activate presence-based debug code. Never infer disabled cheats merely from a settings file that does not persist them. Future timing conclusions must retain this run's confounders rather than silently replacing its history.


On October 15 the owner clarified that the reference is useful only as a source of ideas for our implementation and explicitly declined further upstream debugging. The source build was canceled with its descendants identified and its cancellation receipt preserved; it did not complete. The prepared clean capture remains unexecuted. This closes the reference follow-up scope without claiming a clean comparison or a verified upstream fix. Further work returns to PERF-001 in our runtime.

## Owner original-game shooting-assist verification (2026-10-16 UTC)

The owner independently confirmed that bullets appearing to lock onto enemies
is an original-game mechanic. Do not treat that observation alone as evidence
of an enabled cheat or a defect in the external reference. The owner plans to
compare our version on the next live run and does not remember whether ours
behaves identically. **Our aiming-assist parity remains unverified.** No aiming,
target-selection or projectile behavior was changed for this clarification.
The owner still does not request debugging the external recomp.
