from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST_TEMP_ROOT = ROOT / "build" / "test-tmp"
TEST_TEMP_ROOT.mkdir(parents=True, exist_ok=True)
SOURCE = ROOT / "src" / "evidence" / "g2_trap_probe_runtime.cpp"
TEST = ROOT / "tests" / "g2_trap_probe_runtime_tests.cpp"


class G2TrapProbeRuntimeTests(unittest.TestCase):
    def test_bridge_and_child_isolation_fail_closed(self) -> None:
        # Quarantined on the GitHub Linux runner: the seccomp escape sub-test is
        # environment-incompatible there (reports kSetupFailure before the
        # sandboxed child writes its escape report) while passing under WSL and
        # MSVC. CI sets JFG_QUARANTINE_TRAP_PROBE to skip it; it still runs by
        # default. See docs/development/test-quarantine.md.
        if os.environ.get("JFG_QUARANTINE_TRAP_PROBE") == "1":
            self.skipTest("trap-probe escape test quarantined in this environment")
        compiler = shutil.which("clang++") or shutil.which("g++")
        if compiler is None:
            self.skipTest("a C++20 compiler is unavailable")
        with tempfile.TemporaryDirectory(prefix="g2-trap-probe-runtime-", dir=TEST_TEMP_ROOT) as temporary:
            executable = Path(temporary) / ("g2-trap-probe-runtime-tests.exe" if os.name == "nt" else "g2-trap-probe-runtime-tests")
            built = subprocess.run(
                [
                    compiler,
                    "-std=c++20",
                    "-O2",
                    "-Wall",
                    "-Wextra",
                    "-Wpedantic",
                    "-Werror",
                    "-pthread",
                    "-DJFG_G2_TRAP_PROBE_TESTING=1",
                    "-I",
                    str(ROOT / "include"),
                    str(SOURCE),
                    str(TEST),
                    "-o",
                    str(executable),
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                timeout=120,
            )
            self.assertEqual(built.returncode, 0, built.stderr.decode(errors="replace"))
            ran = subprocess.run(
                [str(executable)],
                cwd=ROOT,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                timeout=30,
            )
            self.assertEqual(ran.returncode, 0, ran.stderr.decode(errors="replace"))


if __name__ == "__main__":
    unittest.main()
