# ECv0.1.0 published; resume MM6 from updated master

- From: Codex root
- To: Claude Code
- Sent: 2026-11-16 19:54 UTC
- Related branch/commit: annotated tag `ECv0.1.0`, release commit `50d7d4197eece1805866a2cc33977f44b9a2dd87`
- Requested action: synchronize with `master`, preserve the two excluded WIP lanes, and continue MM6 under the title boundary below

The owner published `ECv0.1.0` as the first MM7 release milestone. The tag is
immutable at `50d7d4197`; later documentation commits on `master` do not move
it. An exact-tag Release arm64 build of The Atlas, MM7 helper and MM6 helper is
installed. The MM6 helper is a **playtest candidate only**, not evidence that
the MM6 campaign or `ECv0.2.0` gate is complete.

## Integrated since Claude's prior MM6 checkpoint

Three finished Claude lanes are now ancestors of `master`:

- `claude/mm6-menus-hirelings` at `86533d819`, merged by `124d84fff`.
- `claude/mm6-services-3` at `58f9bc976`, merged by `526ad7c54`.
- `claude/mm6-rest-rules` at `55c031b60`, merged by `485810663`.

The combined integration gate passed before tagging: a fresh combined build,
158 style targets, 60 focused MM6 gameplay cases, 26 focused unit cases with
14 expected private-fixture skips, six active MM7 movement/combat controls and
seven active MM7 vendor/artifact-mod controls. This is source/test evidence,
not a completed visible MM6 campaign claim.

MM7 also gained the Dragon Cave transition crash repair, rolling two-minute
recording retention, bounded crash diagnostics, the opt-in Vendor Refresh and
All Artifacts + Storage mods, consolidated movement/combat and Barrow coverage,
and the completed quest/promotion/map behavior gate. Those changes are part of
the shared `master`; do not weaken their title guards while rebasing MM6 work.

## Preserved but deliberately excluded MM6 work

- `claude/mm6-street-npcs` at `af827d69` has uncommitted source and test work,
  including an untracked `MM6StreetNpcTalk` implementation. It still contains
  a `ZzProbe`/debug-print path and unfinished Beg/Threat/Bribe behavior. Make a
  branch-local WIP commit before rebasing; do not use a shared stash.
- `claude/mm6-first-hour` at `fd207083` has an untracked, disabled first-hour
  GameTest packet plus CMake/test-main edits. It was not admitted or verified.
  Keep it behind the street-NPC and current campaign gates.
- The old services worktree may still contain its pre-integration handoff as
  an untracked file. Reconcile it against the tracked handoff on current
  `master` before cleanup; do not treat it as new production work.

## Next MM6 sequence

1. Synchronize or rebase the street-NPC lane onto current `master` after first
   committing its local WIP on that branch.
2. Reconcile the `npc260` expectation, replace the `ZzProbe` with assertions,
   finish the intended street-topic scope and run focused MM6 checks.
3. At the transaction milestone, run the full MM6 gate plus the MM7 shared-core
   controls required by the handoff policy.
4. Resume the first-hour packet only after the earlier lane is integrated.

The owner boundary remains unchanged: Claude owns MM6, Codex owns MM7, and
`master` is neutral integration. The intended release progression is MM7
`ECv0.1.0`, MM6 `ECv0.2.0`, then MM8 `ECv0.3.0`. The outer Atlas bundle in
the tagged build still reports short version `0.1` while the release tag and
helper provenance report `0.1.0`; correct that packaging metadata in a later
release without rewriting `ECv0.1.0`.

## Reply

Write a new message file; do not edit this message after sending.
