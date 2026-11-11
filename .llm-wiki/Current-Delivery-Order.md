---
type: synthesis
up: "[[index_Enroth-Continuum]]"
related: ["[[Agent-Handoff]]", "[[Campaign-Status]]"]
tags: [priorities, mm7, mm6, audit, handoff]
---

# Current Delivery Order

The owner set this order on 2026-11-10 for the personal Mac project; it supersedes older MM6-first backlog ordering without discarding completed MM6 work.

| Order | Outcome | Evidence required before advancing |
| --- | --- | --- |
| 1 | **Get MM7 completely working.** | The current fork candidate, not only the protected installed baseline, launches through The Atlas on the Mac; normal input, media, Save/quit/relaunch/Load and main campaign progression through an ending work with durable local saves and no known blocking defect. Fix issues found in that path. |
| 2 | **Get MM6 working.** | The separate MM6 runtime and profile support normal New Game or Load, visible play, travel and main campaign progression through an ending with durable local saves. Preserve the verified MM7 path. The current seeded first-map headless gate alone does not meet this outcome. |
| 3 | **Run the full MM7 audit.** | Complete the title's [audit checklist](../project/MM7_AUDIT_CHECKLIST.md), including broader rules, maps, optional branches, mods and Mac interface checks beyond the working path; record defects and repairs with appropriate runtime evidence. |
| 4 | **Run the full MM6 audit.** | Complete the [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) and full campaign coverage beyond the working path, with original-behavior comparisons where needed. |

The working gates establish actual use of the current build; the later audits establish breadth. A headless suite, parser inventory or title screen does not close a working gate by itself. After the owner authorized reinstalling Atlas, the prior installed MM7 app was retained as a local rollback copy and the signed candidate installed headlessly. On 2026-11-10 the owner ran the visible session on the installed candidate: title music, NPC voice, combat sound, an NPC conversation, a fight, a new save, relaunch from The Atlas and Load all passed with existing saves unchanged. The MM7 visible and installed-app rows of the working gate are closed. The continuous chapter chain to the Light ending remains. The [November 9 progress table](Progress-2026-11-09.md) is still useful for task facts, but its MM6-first next-step order is superseded by this page.

## Coordination

The owner wants Codex and Claude Code to work in parallel, with Claude taking the planning and coordination lead when it rejoins. Use [agent communications](../agent_comms/README.md) for claims and handoffs, recheck `git worktree list` and the ten-worktree budget, and keep no more than two independent code-changing lanes at a time. Current MM7 lanes are the disabled raid-route test and isolated current-candidate build/save validation; avoid overlapping their files. Integrate reviewed branches into `master` one at a time, retaining protected baseline refs and keeping private GOG material, saves and captures out of Git. Read-only triage may run alongside the code lanes.

See also: [Agent Handoff](Agent-Handoff.md), [Campaign Status](Campaign-Status.md), [November 9 Progress](Progress-2026-11-09.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
