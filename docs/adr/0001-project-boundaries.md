# ADR 0001: Project and data boundaries

Status: accepted; documentation updated for the public source project.

The repository owns project source, build orchestration, synthetic tests,
reviewed manifests, and documentation. Users supply their own supported ROM;
generated game output and private evidence remain outside tracked source.

Original project contributions use the [MIT License](../../LICENSE).
Third-party material retains its [attribution and notices](../../THIRD_PARTY_NOTICES.md).
The source repository does not grant rights to game content or unlicensed
upstream material.

Public pull-request CI uses ROM-free synthetic inputs. Trusted ROM-backed
execution follows the [test-corpus isolation policy](../legal/test-corpus-policy.md).
Input, output, packaging, and report handling follow the
[data policy](../legal/rom-and-assets-policy.md).

Public source availability does not authorize publishing local inputs, private
history, or arbitrary artifacts. Publication remains an owner decision.
Automated work follows [AGENTS.md](../../AGENTS.md) and the production guard.

Earlier private-feasibility and publication decisions are retained in
[Git history](../README.md#historical-material).
