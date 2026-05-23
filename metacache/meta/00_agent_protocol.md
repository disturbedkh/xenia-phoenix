# Agent protocol

How Phoenix agents operate (not 360-specific).

## Canonical trees

- **Code:** `Xenia-Phoenix/xenia-phoenix-src/` — build, edit, PR from here.
- **Memory:** `Xenia-Phoenix/metacache/` — plans, RE notes, runbooks.
- **Reference only:** `../../xenia-canary/` — sibling snapshot; do not treat as build root.
- Do **not** duplicate the source tree under Phoenix.

## Workflow loop

See [dev/00_workflow.md](../dev/00_workflow.md):

1. Pick task from `plan/20_tier_roadmap.md`
2. Read `re/<subsystem>.md`
3. Build / test per `dev/10_*` and `dev/20_*` literally
4. Implement smallest diff in `xenia-phoenix-src`
5. Hand off per [01_handoff.md](01_handoff.md)

## Agent selection

- Default: [agents/orchestrator.md](../agents/orchestrator.md)
- Registry: [agents/registry.yaml](../agents/registry.yaml)

## PR cadence

- Small, single-purpose PRs.
- Template: `xenia-phoenix-src/docs/TIER0_PR_TEMPLATE.md`
- Tags: `[1.1-cpu]`, `[1.2-gpu]`, `[1.3-kernel]`, `[1.4-audio]`, `[debt]`, `[infra]`

## Knowledge capture

After learning anything new about the 360, append to the right `re/*.md` before closing the session.
