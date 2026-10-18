# BUILD-003: Release source validation

Status: resolved for the v1.2.0 release source/build qualification; original failures and platform limits retained below.

Release validation found unregistered controller-prototype rasterizations and launcher screenshots, numeric opcode/section identifiers mistaken for hex payloads, synthetic compiler fixtures mistaken for generated game code, and a repeatedly mis-encoded prototype filename inflating one feature-note line beyond 3 MB. These block publication, not gameplay.

The filename is repaired in the current document; historical evidence remains intact. Exact SHA-256 and Git blob records in `scripts/reviewed_hygiene_bodies.json` document the inspected assets and narrowly scoped historical false positives. No suffix-wide binary exemption or credential/ROM/archive exemption is introduced. Mutated fixtures and artwork must fail again. The controller artwork originates in the owner-provided editable prototype documented beside its SVGs; screenshots show the project launcher/settings.

The all-object local history scan additionally sees private stash objects that are never part of a pushed release. Qualification will scan a clean isolated clone of the committed release history, preserving the local stash and all original counters. No history rewrite is performed. Initial failed policy logs remain in the private release qualification directory.

Lesson: register exact UI asset provenance when adding rasterizations, and keep generated game bodies out of public fixtures. Preserve UTF-8 explicitly when updating documents. Release-history scans must be performed on the actual publishable clone, not confused with private local stash objects.

## Verified candidate scope

Current policy, exact-body mutation regression tests and schema validation pass. The external-output launcher test harness is corrected; the first link probe was missing input_stick.cpp and its corrected link passes. The complete published-history clone remains a pre-push check.

## Full CI portability follow-up

The first release PR CI found LP64 narrowing in the randomized runlink fixture:
std::mt19937 produces 32-bit values but exposes unsigned long on GCC/Linux. The
fixture now explicitly obtains uint32_t words. GCC also requires explicit default
initialization for omitted aggregate pose members; they now have `{}` initializers,
preserving the previous value-initialization behavior. These are compile defects,
not observed gameplay regressions. The initial failed CI run is retained as
38977882410. Full host GCC/CTest qualification is required before the follow-up push.

The follow-up CI passed GCC Debug and Release (99 tests each), then caught signed
fixture indexing under Clang and promoted byte-expression narrowing under the
sanitizer compiler. The fixture explicitly validates/casts nonnegative indices;
the bounded two-byte read explicitly narrows its combined result. The byte values
and camera/reticle behavior are unchanged. Failed run 38978308958 is retained.
Full Clang and sanitizer build/test coverage is required before the next push.

The full local AddressSanitizer + undefined-behavior build passes all 84 tests
in the CI scope (the existing isolated trap-probe quarantine is unchanged).
Clang is not installed in this WSL distribution, so its configuration failed
before compilation. Clang validation remains a required GitHub CI job; no compiler
warning, required check or production guard is disabled to work around that absence.

The next CI run (38979091192) passed policy, both Windows builds, RT64,
launcher and sanitizers; Clang found three further signed controller-fixture
indices. The test already asserts the slot equals 5 before indexing, and now
casts that checked value explicitly. The complete local build then found one
unsigned iterator distance in billboard history; its existing bounds check
remains intact and the distance now uses the vector iterator difference type. A private official Ubuntu Clang 18.1.3
toolchain, downloaded and extracted without installing system packages, then
built the complete host target set with warnings as errors: **100/100 tests
passed** locally. The complete ASan/UBSan scope passed 84/84. No warning or CI
gate was disabled. Final CI and publication remain independent release gates.

Transferable lesson: GCC, Clang and MSVC differ in aggregate and signedness
diagnostics. Run each supported compiler over all fixtures before publication;
checking only the native game translation unit cannot validate the host suite.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.

The final desktop verification initially expected a four-component version
string. The native launcher correctly reports `1.2.0`; its numeric PE fields are
1, 2, 0, 0. The check now verifies those fields and the preserved shortcut
arguments, without changing the published package. Original failure retained
in the private publication receipts.
