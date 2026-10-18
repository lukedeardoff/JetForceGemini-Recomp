# BUILD-001: Restored source and stale generated renderer disagree

- Severity: high in experimental rebuilds. Status: reproduced; coherent rebuild
  and analyzer correction implemented, runtime requalification pending.
- Discovered: 2026-10-14 UTC.
- Affected: private stage-observer and initial semantic-alias experimental builds.
  The delivered route-fix runtime 1b0bdd0f is a separately verified backup and is
  not the mixed build.

## Cause and evidence

The rejected prepared-scene-age experiment changed a shared C++ snapshot layout.
Restoring source with timestamp-preserving copies restored older modification
times. The next incremental build compiled a changed adapter but retained generated
RT64 code/objects expecting the rejected layout. The generated workload source
still referenced prepared_time_us although the current header had no such member.
A private trace reported a render source rate of 864323521, zero-rate displayed
fallbacks and no requested scene screenshots. Exact guest state and host matching
counts did not detect this ABI mismatch.

Private evidence: combat-focus-20261014/stale-generated-build and the initial
stage-dense, alias-dense and room-plane-cross artifacts. Their visual/performance
acceptance is invalid; the separately compiled semantic-history differential
benchmark remains valid. The room-crossing game-state evidence is distinct from
its invalid renderer qualification.

## Remedy and qualification

Explicit CMake configuration regenerates audited RT64 substitutions. Touching
shared headers/adapters and rebuilding compiles both sides against one layout;
generated source is checked for the removed fields before the build.
The trace analyzer now rejects out-of-range source rates in rendered and
displayed records, and reports their distributions. A regression covers corrupt
render-only rates hidden by zero-rate display fallbacks. Runtime requalification
is required before accepting any new candidate or updating the desktop.

## Transferable lesson

A restored executable proves only that executable. It does not repair a later
incremental build directory. After restoring layout or generator inputs, force
configuration and rebuild affected dependencies without preserving obsolete
timestamps. Qualification must validate nonzero interpolation coverage and source
rate sanity as well as game memory, logs and backwards-phase counts.

## Coherent runtime qualification

The explicit coherent rebuild (runtime 731002d5) completes the dense recorded
clock replay with all guest RAM, save bytes and instruction totals equal to the
delivered baseline. Renderer source rates are sane (30/20 in this fixture),
interpolation is active, trace drops are zero and no backward phase is observed.
Later coherent room captures include the actual bridge and exit-light landmarks.
This closes the mixed-build qualification failure for these candidates, not the
owner's open gameplay reports. New layout changes still require coherent rebuilds.
