# Playtesting and reporting

Play normally and report problems that help us improve the game. You do not
need to run developer tests, submit successful checklists, or make a pull request.

## A few things to try

As you play, check that you can reach gameplay, move, jump, aim, shoot, and
return from the pause menu. Watch for obvious graphics or audio problems.
After a normal save opportunity, quit and relaunch to check your progress.
These are suggestions, not a required test session or completion report.

Crashes, freezes, progression blockers, broken controls, incorrect graphics or
audio, and save/load failures are all worth reporting.

## Check whether the problem is already known

Read [known issues](known-issues.md) and search
[existing issues](https://github.com/TK22-26/JetForceGemini-Recomp/issues).
Add to a matching issue when you have useful new information, such as clearer
reproduction steps, a requested hardware comparison, or a failure after a fix.
Keep repeated occurrences of the same problem in that issue.

## Create a report

1. For a freeze, leave the game open and click **Capture freeze** in the launcher.
   Wait for its result, then click **Create support report**. After a crash,
   use **Create support report** directly, reopening the launcher if necessary.
2. Choose the affected session by its time and status. The active session is
   selected while playing; otherwise the newest failed session is suggested.
   Later successful launches do not replace the retained failure.
3. Inspect the ZIP and attach it to the
   [playtest issue form](https://github.com/TK22-26/JetForceGemini-Recomp/issues/new?template=playtest.yml)
   or an existing matching issue. Describe the level/menu, steps/buttons,
   expected and actual behavior, whether it repeats, and whether you loaded a save.

Use one issue per distinct problem. Successful sessions do not need a report.
If capture fails or the button is unavailable, attach the ordinary report and
explain what happened. Games built before 0.4.0-preview.2 need a rebuild to
add crash stacks, recent progress, build metadata and freeze capture.

## What the support ZIP contains

| File | Useful evidence |
|---|---|
| `launcher.log` | UTC session start, launcher revision, stages, exit codes, setup command/step and recognized installer/compiler errors |
| `native.log` | Native stages, exception codes and failure categories |
| `system.log` | Windows build, CPU thread count, GPU vendor/device IDs and driver versions when available |
| `build.log` | Independently selected game source revision, dirty/source-content identity, executable hash, dependency-lock hash and matching local symbol identity |
| `controller.log` | Validated controller mapping and thresholds; no device serial numbers |
| `crash.log` | Fault code and best-effort fault-thread stack: module names, PE identities and relative instruction offsets |
| `hang.log` | On-demand thread stacks from the running native child; capture has a 12-second timeout |
| `breadcrumbs.log` | Up to 60 recent one-second samples of frame, retrace and controller-poll progress |

Each file is limited to 64 KiB and filtered again on export. Empty files mean
the evidence was unavailable. Capture can be incomplete after stack corruption,
forced termination, early startup failures or permission errors. These are
text stack snapshots, not Windows `.dmp` files or a gameplay recording.
The reporter retains ten failed sessions and ten other sessions. ZIPs remain in
`%LOCALAPPDATA%\JFGRecomp\support-exports` until you remove them.

Nothing uploads automatically. The ZIP excludes ROM bytes, generated game code,
saves, personal paths, raw setup/console logs, memory contents and debug symbols.
Attach only the generated ZIP. Describe unknown setup errors in the issue after
removing personal paths; unrecognized stderr stays in the local `setup.log`.
Use [security reporting](../SECURITY.md) for security-sensitive problems.

## After reporting

Keep the affected local build and its matching `.pdb` and `.exe.support` files
until the issue is understood. Keep these files local. A maintainer may ask you
to reproduce a problem or retest a named newer build; reply in the same issue.
The game-source identity may be unavailable for older or manually edited builds;
include `git rev-parse HEAD` if you know the source checkout in that case.

Maintainers: see [support diagnostics](development/support-diagnostics.md) for
the capture format, symbol interpretation and validation procedure.
