# Codex MM7 audit resume preparation — 2026-11-12

- Owner: Codex, sole MM7 arbiter and delivery owner under the owner's 2026-11-12 title split.
- Stage: preparation only. Stage 1 (MM7 working) is closed; the full MM7 audit remains stage 3, after MM6's working gate closes.
- Capacity: Codex has one linked `codex/` worktree in use and may create up to four more after `python3 tools/check_worktree_budget.py --agent codex` succeeds. `master` is neutral integration.

## Retained MM7 audit checkout

The existing `codex/mm7-dark-chain` checkout is deliberately preserved, with
uncommitted work only in these scopes:

- `test/Bin/GameTest/CMakeLists.txt` — registers the new test source.
- `test/Bin/GameTest/GameTests_MM7Promotions.cpp` — adds the Norbert
  Thrush-to-Markham-to-Parson's Quill side-quest scenario.
- `test/Bin/GameTest/GameTests_MM7KerridLetter.cpp` — new Kerrid letter
  dialogue/acquisition scenario.

The working tree passed `git diff --check` during this preparation pass. It was
not built, run, reviewed, committed, or accepted; do not treat it as evidence
that either quest is covered. Preserve it and do not reuse this checkout for
MM6 work.

## Resume order

1. Keep the delivery order: finish MM6 working first; do not advance MM7 audit
   implementation merely because this audit checkout exists.
2. At stage 3 start, rebase this MM7 branch onto then-current `master`, inspect
   the three-file diff, build it, and run the two focused side-quest tests with
   meaningful refusal/state assertions before any review.
3. Reconcile each successful test with
   [the MM7 quest behavior catalog](../../.llm-wiki/MM7-Quest-Behavior-Catalog.md),
   then use its contract order: repair the Wine Cellar oracle, build the Dark
   C1-C7 durable chain, and complete regional side/promotion batches.
4. Use the MM7 audit checklist for the broader save, rules, Mac-interface,
   mods and observed-play evidence. Keep all private game data, profiles,
   saves, captures and raw logs outside Git.

See also: [Current Delivery Order](../../.llm-wiki/Current-Delivery-Order.md),
[Agent Handoff](../../.llm-wiki/Agent-Handoff.md),
[MM7 Audit Checklist](../../project/MM7_AUDIT_CHECKLIST.md), and
[Codex integration claim](../claims/codex-integration.md).
