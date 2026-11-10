# Claude lead — end of week handoff, 2026-11-10

- Owner: Claude Code, planning and coordination lead. Recipient: Codex root and the next Claude session.
- The owner ended the session for the week. The living plan is `agent_comms/PLAN.md`. This handoff records where every Claude-owned item stands.

## Stage 1 — MM7 working gate

| Row | State |
| --- | --- |
| W1 | Passed. Staged current-source build, Save and fresh-process Load. |
| W2 | Passed. Harmondale raid test integrated. |
| W3 | Staged visible session passed most steps. Remaining: audible music and effects, a roaming NPC talk, a monster fight. Folded into owner session S1 in `PLAN.md`. Needs owner display time. |
| W4 | Every Light-path chapter is input-tested from its own setup, both chapter seams survive a fresh process, and the Xenofex kill is now a real fight (waiting for integration). Remaining: the continuous chain (Codex harness and C0 passed 1/1 on `codex/mm7-chain-harness` at `a30db1654`, waiting for Claude review, C1-C7 and the missing-C0 negative not started), real acquisition of the remaining quest items (Claude lane paused, below), travel by teleport. |
| W5 | Signed candidate installed headlessly with a rollback copy. Visible relaunch and Load waits for session S1. |
| W6 | No player-blocking defect is open. The suspected NPC topic loss on Load was the hand-in itself, not a save defect. |

## Claude work this session

Integrated by Codex after Claude review:
- Item-target scroll kept when its item pick is cancelled (`7b3b7b074`).
- Actor- and character-target scrolls kept when targeting is cancelled, with the inventory-screen read path tested (`ece2203eb`, `374017cdf`).
- Master full `check_style` fixed, 75 errors in about 55 files (`9c88547b3`).

Reviewed and waiting for integration:
- `claude/mm7-xenofex-combat` at `0cd1d14aa`. Handoff `handoffs/2026-11-10-claude-mm7-xenofex-combat.md`.

Paused:
- `claude/mm7-light-items` in the `mm7-save-durability` checkout. WIP commit `88cbca0f5`, not built or run. It adds `GameTests_MM7LightItems.cpp` with three draft tests whose expected tapes are first guesses: `WallsPillarKeysFromChests` (three keys from Walls of Mist chests 2-4, then the pedestals and exit doors), `AltarPiecesFromTempleChests` (Light piece from the Temple of the Light, Dark piece from the Temple of the Dark, hand-in refused with one piece and accepted with both) and `SoulJarsFromGloamingChest` (refused without the case, accepted after looting Castle Gloaming chest 0). Every source chest was confirmed by a runtime probe. The Wine Cellar kill is not started. Its map exit sets the kill bit unconditionally, so only a Town Portal style exit depends on the kill. Next: build, run, fix the tapes, behavior-guard checks, cpplint, Light controls, full MM7 binary. Hostile temple and castle groups may hurt the level-1 party, which may need the Xenofex strength fixture.

Planning and review:
- `PLAN.md`: stage 1 rows W1-W6, owner session S1, the W4 coverage map, the continuous chain proposal with its slot policy, the defect queue, stage 2 rows M1-M8 with the MM6 main-quest risk probe and its runtime timer confirmation.
- Reviews: `reviews/2026-11-10-claude-mm7-w4-path-tests.md` (F1-F6, all closed except F6 comments in Light files not yet cleaned), approvals of the Walls seam, the Pit-to-Xenofex seam and the tape cleanup.

## Next actions

1. Owner: session S1 when display time allows. It closes W3 and W5.
2. Claude: review `codex/mm7-chain-harness` (`a30db1654`, handoff `handoffs/2026-11-10-codex-mm7-chain-harness-c0.md` on that branch), including the kept-slot policy and the missing-C0 refusal, which has not run yet. Codex: integrate it and `claude/mm7-xenofex-combat` one at a time.
3. Next Claude session: resume `claude/mm7-light-items` from its WIP commit, then take a C4-C7 chain chapter lane once the harness lands.
4. Stage 1 closes when S1 passes and the chain runs C0-C7 to the Light ending. Then stage 2 starts with S-NG and S-SWEEP per `PLAN.md`.

## Worktrees and limits

8 of 10 worktrees registered. The `mm7-save-durability` checkout holds the Claude branches `claude/mm7-scroll-cancel`, `claude/mm7-actor-scroll-cancel`, `claude/master-style-fix`, `claude/mm7-xenofex-combat` and `claude/mm7-light-items`. Keep the refs until each is integrated. 19 GiB free at wrap-up.

Privacy: no game data, saves, captures or logs in Git. Private probe outputs stay under `local-game-data/`.
