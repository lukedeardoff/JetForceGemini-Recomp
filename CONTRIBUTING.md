# Contributing

The most useful way to help right now is to play the preview and report
problems. Code contributions are also welcome through reviewed pull requests.

## Help by playtesting

Follow [getting started](docs/getting-started.md), then play normally.
Check [known issues](docs/known-issues.md) before reporting a problem and use
[playtesting and reporting](docs/playtesting.md) to attach the launcher's
support ZIP to GitHub. Successful sessions do not require a report.

You do not need a development branch, a pull request, or the developer test
suite to report a bug. Clear steps, the affected build, and useful diagnostics
help us reproduce it. Retesting a specific fix is valuable too.

## Core fidelity and community enhancements

The base repository prioritizes a faithful 1:1 recreation of the original
N64 game's behavior and rendering. Fidelity fixes, compatibility fixes,
and build or diagnostic improvements support that goal.

HD textures, cosmetic shaders, replacement artwork, and other visual
enhancements belong in independently maintained community branches.
Their authors are welcome to keep developing and sharing them. The base
repository does not collect or maintain those variants. Future mod support
is a separate project; it is not a prerequisite for contributing a focused
correctness fix today.

## Contribute code

1. Check existing issues and discuss substantial changes before implementing
   them. Keep a first contribution focused on one problem.
2. Create a feature branch. Keep compatibility fixes separate from enhancements.
3. Explain the problem, the change, and how you verified it. Add regression
   coverage for behavior changes where practical, using fixtures without game data.
4. Follow [development setup](docs/development/setup.md) for relevant checks.
   Run `python scripts/check_repository_hygiene.py --history` before pushing.
   Documentation changes should include link and whitespace checks and follow
   the [documentation layout](docs/README.md#where-documents-belong); runtime
   changes need the applicable builds and tests.
5. Open a pull request. A draft is welcome while work is in progress. Include
   test results and any remaining limitations; maintainers will review before merging.

Preserve existing user files and saves. Do not change golden baselines to hide
a failure or mix generated game output with handwritten source.

## Authorship and credit

Use your own name and chosen email when committing; do not use the maintainer's
identity. A GitHub noreply address is recommended if you want to keep your email
private. Both `username@users.noreply.github.com` and GitHub's
`numeric-id+username@users.noreply.github.com` format are accepted.
Contributors may keep their original commit timezones. The maintainer's exact
noreply identity and UTC timestamps remain required by [AGENTS.md](AGENTS.md).
Existing host-generated merge history retains its documented exception.

Preserve original authorship when carrying contributor commits forward, and
credit reports and investigations even when the final implementation differs.
See [community credits](CREDITS.md). Credit does not require rewriting history
or attributing independently written code to someone who did not write it.

## Keep submissions focused

Submit original contributions under the [MIT License](LICENSE) and preserve
[third-party notices](THIRD_PARTY_NOTICES.md). Keep ROMs, game assets, generated
game code, and saves out of commits and attachments. For bug reports, use the
launcher-generated ZIP described in the [reporting guide](docs/playtesting.md).
See [data handling](docs/legal/rom-and-assets-policy.md) for detailed rules.

Review any AI-assisted changes before submitting them. Contributions to
upstream dependencies must follow their own policies;
the evaluated N64 recompilation projects do not accept AI-generated upstream
contributions. Do not present AI-assisted work as human-authored upstream.

## Automated development

Agents must follow [AGENTS.md](AGENTS.md) and the production
[guard contract](docs/planning/autonomy-progress-guard.md). Preserve existing
investigation accounting, run automated work through the guarded supervisor,
and stop when it denies further execution.
