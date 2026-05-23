# XeniOS reference notes

**Repo:** [XeniOS](../../XeniOS/) — branch `xenios` (`origin/xenios`)

**Status (2026-05-23):** **Dormant.** HEAD `056d2f64` (2026-03-14) — `[Release] Default marketing version to 1.0.1`. No commits since March 2026.

## Role for Phoenix

Tag XeniOS as the **reference fork for a future iOS host port** — not an active merge source today.

| Area | XeniOS value | Phoenix status |
|------|--------------|----------------|
| Apple platform packaging | App Store / iOS shell patterns | Phase 4.4a macOS landed; **no iOS phase yet** |
| MoltenVK / Metal | May overlap Edge Metal backend | Deferred EPQ-09 / Phase 4.4a |
| JIT / memory on iOS | Platform constraints | Unplanned |

## Proposed roadmap slot

Add **Phase 5 — iOS** to [20_tier_roadmap.md](20_tier_roadmap.md) when macOS Phase 4.4a CI smoke is green and Android Phase 3 device QA completes. Phase 5 entry criteria:

1. Green macOS universal binary CI (Edge has this; Phoenix tracks via 4.4a).
2. Legal/controller input story on iOS (GameController.framework).
3. Read-only audit of XeniOS `xenios` branch for shell + build matrix — **no blind cherry-picks**.

## Sync policy

- `git fetch origin xenios` on session start (reference freshness).
- Do **not** cherry-pick from XeniOS until Phase 5 is explicitly opened.
- Cross-check Edge Metal/MoltenVK commits before any iOS GPU work — Edge is more current than XeniOS.

## References

- [macos_compat_gap_analysis.md](macos_compat_gap_analysis.md)
- [edge_port_queue.md](edge_port_queue.md) EPQ-09 (Metal backend)
