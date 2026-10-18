# Quarantined tests

Quarantined CTest cases still build and run by default (locally and for
developers); CI excludes them by name with `ctest -E "<regex>"`. Exclusion is
done in the workflow rather than with a CTest `LABELS` property on purpose:
`tests/CMakeLists.txt` is byte-pinned in the Phase 4 compiler-replay build
closure (`scripts/phase4_evidence_harness.py`), so editing it to add a label
would invalidate the signed completion evidence. Quarantine is a
release-blocking state: a quarantined test must be root-caused and restored,
never left disabled silently, and never deleted to make CI green.

Some tests have a second entry point: `tests/test_g2_trap_probe_runtime.py`
compiles and runs the same C++ executable from the policy lane's `unittest`
sweep, independently of CTest. That wrapper skips when
`JFG_QUARANTINE_TRAP_PROBE=1`, which the CI policy step sets; it still runs by
default everywhere else. Both entry points must be re-enabled together when the
root cause is fixed.

## jfg.g2_trap_probe_runtime

- **Quarantined:** 2026-08-08
- **Symptom:** the seccomp escape sub-test (step 14) fails on the GitHub
  `ubuntu-24.04` runner with `escape probe: report_bytes=0 outcome=6
  child_status=0` — i.e. `run_isolated` returns `kSetupFailure` and the
  sandboxed child never writes its escape report — within ~1 second. It passes
  reliably under WSL Ubuntu-24.04 (60/60 stress runs) and on Windows/MSVC.
- **Scope:** this is the trap-probe hardening unit test only. It is **not** part
  of the Phase 4 completion evidence chain; the pinned G2 runtime-trap producer
  and its native/oracle records are validated separately and pass.
- **Ruled out:** deadline too tight (failure is ~1s, far under the functional
  and escape deadlines); process-group teardown liveness (raising the 1s
  cleanup bound to 15s did not change the ~1s failure).
- **Leading hypothesis:** the GitHub runner kernel's seccomp/credential-passing
  or process-group behavior differs from the WSL kernel such that the escape
  child's setup or report path is torn down before it completes. Needs
  reproduction on a stock cloud Linux kernel (the environment CI uses) rather
  than the WSL kernel.
- **Exit criteria:** reproduce on a GitHub-equivalent kernel, fix the
  environment interaction without weakening the syscall-denial assertions
  (each escape syscall must still return `-1`/`EPERM`, seccomp mode
  `FILTER`), then remove the CI `-E` exclusion, the `JFG_QUARANTINE_TRAP_PROBE`
  setting and this entry.
- **Do not:** relax the escape assertions or delete the sub-test to restore it.
- **Scope of the exclusion (2026-09-01):** the `-E` exclusion applies only to
  the Linux build lanes and the Linux sanitizer lane. The Windows lanes run
  the test, since the failure has never reproduced there; a Windows failure
  is therefore a real regression, not this quarantine.
