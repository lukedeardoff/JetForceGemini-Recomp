# Pinned upstream baseline

`dependencies.lock.json` is authoritative for every pin, submodule and tool
record; this table is a readable summary.

| Component | Commit | Use |
|---|---|---|
| JFG decomp | `b49aa4791e8fb1e7acd3bba10346876358d0a9a7` | Local ELF build and symbol facts only |
| N64Recomp | `ffb39cdad1da5de07eaaa48bd1db4a89a7986771` | Local generator, with the maintained [patch series](../../patches/n64recomp/README.md) |
| RT64 | `5473732a822a4423b5696e7cb18fecc425a59875` | Native renderer |
| N64ModernRuntime | `589bbf018a3e6d3646ddf7de1e7919f1b7e99bb1` | Evaluated reference; not the project's runtime |
| RecompFrontend | `9ef9cdfdead7649247ab4957f43517f44c33931d` | Excluded pending permission |

Licensing for each is in the [dependency inventory](../legal/dependency-license-inventory.md).
The decomp build needs Linux/WSL and the user-supplied US retail ROM with SHA-1
`493ced9008dbe932d6e91179b68e8630cf23a023`. Newer upstream observations are
in the [reference review](reference-review-2026-10-01.md).

## Phase 1 equivalence record

Phase 1 reproduced the decomp build in clean Ubuntu 22.04 and 24.04 WSL2
environments, each performing setup, extraction and two clean verified builds.
The public result is [phase1-build-evidence.json](phase1-build-evidence.json)
(its path is fixed by its schema and validator). Two environments are
equivalent only when the ROM SHA-1, ELF SHA-256, all 326 ELF section records
and normalized section hashes, symbol/binding/relocation counts and the
aggregate build report match; defined symbols agree; allocated sections do not
overlap; and the 157-slot overlay table and 155 populated spans pass bounds,
identity and range checks. Overlay relocation-size fields are byte lengths,
not entry counts.

Linker-map formats differ between binutils 2.38 and 2.42, so raw map hashes are
recorded per environment while parsed results must agree. Map coverage is
complete for sections (167 of 167) and partial for symbols (9,933 of 14,603);
the rest are covered by the hashed ELF symbol table. Overlay table coordinates
come from a private layout file and are never tracked.

CI validates the aggregate's schema, arithmetic and verdict; the clean builds
themselves are a maintainer attestation because the ROM-dependent artifacts are
not tracked:

```text
python3 scripts/compare_phase1_evidence.py --validate-aggregate docs/upstream/phase1-build-evidence.json
```

Project-owned Phase 1 helpers live in `scripts/` (`build_upstream.py`,
`validate_elf.py`, `parse_upstream_report.py`, `summarize_overlays.py`,
`compare_phase1_evidence.py`) because `tools/` is reserved for ignored material.
