# Stop conditions

Stop and ask the human if any of these are true:

- A change would touch **>100 files** in one PR.
- A change **deletes a kernel shim** that has callers.
- A change modifies the patcher schema (`src/xenia/patcher/patch_db.h`) without a migration.
- A **vmx128-fuzz divergence** cannot be explained after 30 minutes of investigation.
- A PR moves **none** of the Phoenix metrics (see [03_metrics.md](03_metrics.md)) — document why in `plan/50_open_questions.md` first.

## Agent-specific

See **Stop and ask human** section in your [agents/<id>.md](../agents/) card.
