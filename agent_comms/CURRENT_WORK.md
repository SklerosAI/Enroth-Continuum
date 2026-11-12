# Current work board — 2026-11-12

The owner's delivery order remains MM7 working, MM6 working, full MM7 audit,
then full MM6 audit. Stage 1 is closed and Stage 2 is active. This board is the
current coordination surface; detailed historical lane results remain in
`agent_comms/handoffs/` and `PLAN.md`.

## Title authority and worktree budgets

- Codex is the sole arbiter and delivery owner for MM7. Codex assigns and
  accepts MM7 work.
- Claude Code is the sole arbiter and delivery owner for MM6. Claude assigns
  and accepts MM6 work.
- Each owner may use up to five linked worktrees. Neutral `master` consumes
  neither allocation. Run
  `python3 tools/check_worktree_budget.py --agent <codex|claude>` before adding
  a checkout.

Current allocation: Codex 1/5 (`mm7-combat-fixes` on
`codex/mm7-dark-chain`); Claude 3/5 (`mm6-houses`, `mm6-hud`, and
`mm6-npc-dialogue`). All four worktrees are title-aligned. Neutral `master`
and the Codex MM7 branch are kept synchronized after each integrated packet.

## Active and queued work

| Title | Owner | Current state | Next coordination point |
| --- | --- | --- | --- |
| MM7 | Codex | Working gate closed. Eight independent regional routes now have exact production-Save/fresh-process-Load checkpoints. The latest Obelisk packet passed its focused route, writer, reader, admission refusals, regional 14/14 gate and style 157/157. | Continue from the [remaining quest and area list](../project/MM7_REMAINING_QUESTS_AND_AREAS.md). The next reserved slice is the war-alternatives matrix; keep it independent from continuous-campaign claims. |
| MM7 graphics | Codex | The malformed Tatalia/Avlee progress frames were traced to diagnostic placement and the headless fallback is covered. Shoals fog and corrected frames remain visually unclassified. | Do not run graphical recapture while the owner's headless-only instruction remains active. Inspect every future image before accepting it as evidence. |
| MM6 | Claude Code | Stage 2 remains active under Claude's title authority, with three linked worktrees currently allocated. | Claude controls MM6 scope, review, acceptance and further worktree use; Codex only protects MM7 at neutral integration seams. |
| Integration | Both title owners | `master` is neutral. A title owner reconciles current `master`, runs the other title's required protection controls when shared core changes, and integrates only reviewed work from its own title. | Preserve clean title branches and handoffs; do not use cross-title authority to resolve scope disputes. |

## Superseded lane labels

All earlier entries that named Claude as an MM7 lead, gave Codex authority over
MM6, or listed completed Stage-1 MM7 work as active are historical only. Their
test evidence remains valid where current source still agrees, but they do not
override the strict title split above.

