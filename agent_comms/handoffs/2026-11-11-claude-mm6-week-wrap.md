# Claude — MM6 week wrap, 2026-11-11

- Owner: Claude Code, MM6 owner since the owner's assignment on 2026-11-11 (claim `claims/claude-mm6-all.md`). Codex leads and owns MM7.
- Recipient: the owner, Codex, and the next Claude session.

## On master today

All of these passed the full MM6 suite and the full headless MM7 suite (540/540) on their merged build, unless noted.

| Lane | What it delivered | Commit |
| --- | --- | --- |
| Captures | `MM6OffscreenFrame.DISABLED_MapCapture` and `MM7OffscreenFrame.DISABLED_MapCapture` render any map offscreen, for the progress captures | `77b42b687` |
| H | MM6 campaign chain harness: `--title mm6` in `tools/run_mm7_campaign_chain.py`, `--mm6-chain-profile`, `MM6CampaignChain` helper, chapter C0 | `133fabf5a` |
| SKY | MM6 outdoor skies follow MM6's loader rule instead of MM7's, no more placeholder sky | `98cb96ab3` |
| X | MM6 house exits with quest-bit restrictions and arrivals, confirmation view, the King's Library room chain | `cb179e000` |
| F | MM6 hive finale, winning and losing endings, MM6 blasters fire the laser bolt with Energy damage | `40b68b45e` |
| C1 | Chain chapter C1 (councils A and C), the opening letter for a new party, map parking on test teleports | `00ddf8d5a` (merged by Codex), handoff `ae498b2d1` |
| BOW | MM6 bows fire object 500, the MM6 arrow. They never fired before | `0705a8173` (merged by Codex) |
| VISIT | MM6 map visit record read and written as MM6 does, with MM6's map timer rule | `e27757e2f` (merged by Codex), handoff `6a62d1903`. Lane checks: MM6 suite 119, MM7 controls 107, first-map gate 33/33 |

Codex installed an Atlas with an MM6 button (`Profiles/MM6-Atlas`). A separate MM6 event-fixture branch was reviewed but reverted because its synthetic records are incompatible with the current event parser; it is not part of the playtest baseline.

## Final check on master

On `e27757e2f`, the last code commit of the day, built from scratch in its own worktree: `check_style` clean, unit tests 1,122 passed, MM6 suite 132 passed, full headless MM7 suite 540/540. This covers every lane above and Codex's MM6 merges together.

## Queue for next week, most player-blocking first

1. Chain C2 (council B): Newton on oute2, Gharik's key from d07, the Hourglass from d18. Starts from C1's `save001.ec6` on outd1. Risks: a d18 timer damage kind the MM6 guard refuses, and no chapter has driven MM6 indoor levers yet. Handoff `handoffs/2026-11-11-claude-mm6-chain-c1-c4.md`.
2. Party death after a real map change: once, the respawn to oute3 threw the chest generation guard. Recheck with a test.
3. MM6 arrows and monster missiles 500-590 may render as azure sparkles instead of their sprites (`SpellFxRenderer::RenderAsSprite`). Needs a real render check.
4. Percival arrows (item 405) turn into object 600, which MM6 lacks. The original uses 1051 with a radius 512 area attack.
5. C3 blockers: MM6 street dialogue (SpeakNPC, sewer:8) and the Hermit needing Fly or a teleport fixture. C4: the Devil Outpost fight and the t7 keys.
6. The 16 guards the reactor summons stood idle in a probe. Check the original.
7. Traitor step: MM6 re-enters house 165 with a one-time `Citytrtr` movie after the MoveNPC. Not built, cosmetic.
8. Bright white ground near the oute3 start and in patches on outd3.
9. Visit record gaps (handoff `handoffs/2026-11-11-claude-mm6-visit-record.md`): a first-map v5 save that never left oute3 is not stamped, MM6's per-firing timer catch-up differs from the port's `checkTimer`, and the Sorpigal fountain took 46 clicks to drain where 30 were expected. Seven MM6 `OnTimer` triggers without an interval no longer fire on map entry, following `MM6.exe`.
10. Shared party-creation helper for tests (duplicated in the chain C0 and the house exit tests).
11. Rename `tools/run_mm7_campaign_chain.py`, it runs both titles now.

## Machine and tools

- `~/enroth-tools/bin/enroth-build` is at SLOTS=1 since the midday swap pressure (load ~400). Raise it back to 2 only when swap stays below about 4 GB.
- `~/enroth-tools/bin/enroth-capture-loop` tours every MM6 and MM7 map every 3 minutes for up to 12 hours, only from a build that contains master's latest code. Stop it with `touch /private/tmp/enroth-capture-loop.stop`.
- `build-release/` in the main checkout holds a Release MM6 for the local `play-mm6` utility, on `Profiles/MM6`; the former Desktop developer shortcut was removed.
- Worktrees: 10 of 10. Free for reuse: `mm7-monster-combat`, `mm7-save-durability`, `mm7-load-admission`, `mm7-death-regression`, `mm7-mod-access`, `mm7-rolling-replay` and the detached `mm7-save-atomic`.

Privacy: no game data, saves, captures, logs or disassembly in Git.
