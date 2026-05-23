# Legal and ethics

## No piracy enablement

- No bundled keys, no DRM bypass, no decryption utilities beyond what canary upstream already ships.
- Tools that ingest user-owned media must say so in CLI help.
- Do not commit ISO paths, keys, or user `telemetry/` captures.

## NDA / leaked material

- Xbox 360 XDK docs may exist in leaked form — use as **RE reference only**; paraphrase + cite, never paste into the repo.

## Upstream ethics

- Phoenix is a fork, not a hijack. Generically good changes should be proposed upstream to [xenia-canary](https://github.com/xenia-canary/xenia-canary).
- Keep diffs understandable; [dev/30_repo_layout.md](../dev/30_repo_layout.md) must stay accurate.

## Telemetry

Structured JSONL only — see [schemas/telemetry.md](../schemas/telemetry.md) (stub hits, XMA, PCM, **obs events** §8). Prefer `obs::Invariant` / `EmitEvent` for new GPU failure modes. No ad-hoc `XELOGE` as a substitute for stub tracing.

## Cursor live probe

- User-owned game paths only (`metacache/cache/games.local.yaml`, gitignored).
- `phoenix_probe` HTTP binds **127.0.0.1** only; default port **0** (disabled). See [schemas/phoenix_probe_http.md](../schemas/phoenix_probe_http.md).
- MCP responses must not leak full ISO paths (basename only unless the user supplied the path in that request).
