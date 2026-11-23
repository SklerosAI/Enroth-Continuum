---
type: index
up: "[[Home_Enroth-Continuum]]"
tags: [handoff, agents, mm7, mm6]
---

# Agent Handoff

The starting brief for an agent picking up Enroth Continuum. Check it against the repository before acting.

## Read first

1. Run `git status --short --branch`, `git worktree list` and `git log -1 --oneline` in the checkout you mean to use.
2. Read [AGENTS.md](../AGENTS.md), `agent_comms/CURRENT_WORK.md` and the [Current Delivery Order](Current-Delivery-Order.md).
3. For MM7 read [Campaign Status](Campaign-Status.md), [MM7 Playthrough Go/No-Go](MM7-Playthrough-Go-No-Go.md),
   [Open Questions](Open-Questions.md) and the [audit checklist](../project/MM7_AUDIT_CHECKLIST.md). For MM6 switch to
   `claude/mm6` and read its `docs/index.md`.
4. For the engine read the [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), and for what
   may enter Git the [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).

## Current state — 2026-11-23

- **MM7** is owned by Codex on `codex/mm7`, in the main checkout. Codex is idle since 2026-11-22. The working gate is
  closed, `ECv0.1.0` and `ECv0.1.1` are tagged, and the full audit has not started. See [Campaign Status](Campaign-Status.md).
- **MM6** is owned by Claude Code on `claude/mm6`, in `enroth-worktrees/mm6-control-center`, pushed to `origin`.
  Its curated `docs/` is the MM6 source of truth. The MM6 pages and `project/MM6_*` packets on this branch are not current.
- **MM8** is reserved. The Atlas detects its data and offers no Play action.
- **Installed app.** The Atlas carries Release helpers from MM7 `014514250` and MM6 `626f5f431`, installed 2026-11-22,
  see [Release Milestones](Release-Milestones.md#local-mm6mm7-playtest-app--2026-11-22).

## App replacement policy — 2026-11-21

Unless the owner says otherwise, every app rebuild removes all superseded installed and staged app bundles once the
replacement is verified. No old app version is kept for rollback. Saved games are repaired or migrated for the new
build when needed. The policy does not by itself allow deleting profiles or saves.

## Title ownership and checkouts

Codex is the sole arbiter and delivery owner for MM7: the working gate, audit, defects, acceptance evidence and lane
decisions. Claude Code owns all MM6 work on the same terms. Neither may redirect, accept or change the other title's
work without a new owner instruction. `master` is neutral integration.

While a round runs there is no cap on worktrees or branches, and at least 10 GB of disk stays free. A round ends with
one branch and one worktree per title. Use `codex/` and `claude/` branch prefixes. `tools/check_worktree_budget.py`
still enforces an older five-per-owner cap, so read its counts, not its verdict. Never build an MM6 helper into the MM7
slot or touch another title's profile, see [Runtime Isolation](Runtime-Isolation.md).

## Evidence and reporting standard

Label evidence precisely. **Inventory** means found or indexed. **Parse** means bounded decoding. **Semantic** means a
field or rule meaning checked against evidence. **Offline implementation** means title-specific effects executed in
focused tests. **Runtime** means seen or heard in the installed engine. **Campaign** means progression and durable
save and reload observed. Advance a checklist item only at the level its wording requires.

A finding reports its source and locator, method and commit, the observed fact, the engine consumer, confidence, the
change or acceptance check it needs, and its privacy class. Per-ID tables, decoded data, hashes, save bytes, captures
of original game content and command logs stay in ignored local storage. Public pages state conclusions.

The owner's shipped GOG data settles exact content and IDs and stays private. Guides explain intended mechanics and are
reconciled against the data and the engine. Source shows what the engine does now, not that the game is correct.
Record a disagreement with its source, date and confidence instead of silently picking one side.

## Attribution

Preserve OpenEnroth history, LGPL notices and contributor credit. See [CREDITS.md](../CREDITS.md),
[project/UPSTREAM.md](../project/UPSTREAM.md) and [LICENSE](../LICENSE).

See also: [Current Delivery Order](Current-Delivery-Order.md), [Campaign Status](Campaign-Status.md), [Open Questions](Open-Questions.md), [Release Milestones](Release-Milestones.md), [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md), [Private Companion Wiki](Private-Companion-Wiki.md).
