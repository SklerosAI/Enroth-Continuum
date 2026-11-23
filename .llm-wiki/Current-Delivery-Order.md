---
type: synthesis
up: "[[index_Enroth-Continuum]]"
related: ["[[Agent-Handoff]]", "[[Campaign-Status]]"]
tags: [priorities, mm7, mm6, audit, handoff]
---

# Current Delivery Order

The owner's order for the project. Each stage advances only on the evidence in its row.

| Order | Outcome | State | Evidence required before advancing |
| --- | --- | --- | --- |
| 1 | **Get MM7 completely working.** | Done 2026-11-11 | The current build launches through the Atlas on the Mac, and normal input, media, Save, quit, relaunch, Load and the main campaign through an ending work with durable local saves and no known blocking defect. |
| 2 | **Get MM6 working.** | Active | The separate MM6 runtime and profile support New Game or Load, visible play, travel and the main campaign through an ending with durable local saves, without breaking MM7. The readiness flag is `docs/mm6-playthrough-go-no-go.md` on `claude/mm6`. |
| 3 | **Run the full MM7 audit.** | Not started | Complete the [audit checklist](../project/MM7_AUDIT_CHECKLIST.md): rules, maps, optional branches, mods and Mac interface checks beyond the working path, with defects and repairs recorded with runtime evidence. The [delta-first plan](../project/MM7_FULL_AUDIT_NEXT_WEEK_PLAN.md) names the week of 2026-11-23, and the start is the owner's call. |
| 4 | **Run the full MM6 audit.** | Not started | Full MM6 campaign and rules coverage beyond the working path, with original-behaviour comparisons where needed. Tracked on `claude/mm6`. |

A working gate means actual use of the current build, and the audits establish breadth. A headless suite, a parser
inventory or a title screen does not close a working gate by itself.

## Coordination

- **Title authority.** Codex is the sole arbiter and delivery owner for MM7: the working gate, the audit, MM7 defects,
  acceptance evidence, lanes and integration. Claude Code owns all MM6 work on the same terms, on `claude/mm6`.
  Neither may redirect, accept or change the other title's work without an explicit owner instruction. Codex is idle
  since 2026-11-22.
- **Worktrees.** No worktree or branch count cap while a round is running. Keep at least 10 GB free disk, a hard
  floor. Each round ends with one branch and one worktree per title: the main checkout on `codex/mm7` and
  `enroth-worktrees/mm6-control-center` on `claude/mm6`. `tools/check_worktree_budget.py` still enforces an older
  five-per-owner cap, so read its counts, not its verdict.
- **Integration.** `master` is the neutral integration branch. A shared-core change runs the other title's controls
  before it merges. Live notes between the agents go in the ignored `agent_comms/live/` mailbox, durable claims and
  handoffs in [agent communications](../agent_comms/README.md), and the board is `agent_comms/CURRENT_WORK.md`.
- **Privacy.** Private game data, saves and captures stay out of Git, see the
  [source and privacy boundary](Source-and-Privacy-Boundary.md).

See also: [Agent Handoff](Agent-Handoff.md), [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
