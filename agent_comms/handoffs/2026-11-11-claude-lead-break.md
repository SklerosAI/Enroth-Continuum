# Claude lead — break handoff, 2026-11-11

- Owner: Claude Code, planning and coordination lead. Recipient: the owner, Codex on its return, and the next Claude session.
- Living plan: `agent_comms/PLAN.md`. Codex has been silent since 2026-11-11 03:22 UTC, so Claude integrated everything below.

## Stage 1, MM7 working: closed

All rows W1-W6 passed. The owner's visible session (S1) passed on the installed app, and the continuous chain C0-C7 runs from New Game to the Light ending through real saves. The full headless MM7 suite passes 540/540 after every merge since.

## Stage 2, MM6 working: on `master` through `0b7f6b255`/`2c5d0bdd2`

Integrated and validated, all following the original MM6 executable:
- Production New Game, saved, loaded in a fresh process and saved again (M1).
- All 67 maps load and all 26 main-quest maps run every timer and event (event sweep), and survive monster attacks (combat sweep: 0 of 26 throwing).
- Award animation crash, reputation variable, generated NPC pool, damage channels, the indoor decoration parser (t7 trap loop), event spells and the d18 vents, monster missiles, spells and special attacks.
- Edge travel, dungeon doors, maps kept across a return, Save and Load on any map (EC6 v7), autosave where the original writes it, quick save and quick load, and in-game Load (M2).
- Party death and respawn at the New Sorpigal fountain (M3).
- Chest traps, MM6's own interactive decorations (hover crash), boats and stables with the stable fix-prices quest step.
- Last full run on `master` (autosave integration): MM6 suite 115, unit tests 1,120, MM6 first-map gate 32 with the MM7 controls, MM7 in-process 107, full MM7 540/540, `check_style` clean. The event sweep at handoff: 67 maps loaded, 66 finished, no crash. The combat sweep was still finishing.

## Paused lanes (work in progress, not on master)

All three stopped cleanly with committed WIP. None of their code is built or tested yet.

- **F, hive finale**, `claude/mm6-finale` in `mm7-save-durability`, `789ca52ee` (research only, with a handoff on the branch). From the executable: killing monster 172 sets qbit 202. Killing reactor 173 without item 544 loses at once (house 601). With item 544 it sets qbit 180, summons 15 monsters, sets five doors and moves the party. The reactor is immune to everything but Energy, so blasters are a data requirement. There is no reactor timer. `hive:60` with item 544 wins (house 600: movies, the win screen with score, then a box and a move to `oute3`), without it loses. During the hive state, death, leaving the map and autosave all lead to the losing ending or are skipped. Next: `MM6Finale.{h,cpp}`, an MM6 game-over window, hooks, tests.
- **H, MM6 chain harness**, `claude/mm6-chain-harness` in `mm7-monster-combat`, `0a138f7a4` (runner title switch, `--title mm6`) and `4223e449a` (GameTestMain MM6 chain rows, `--mm6-chain-profile`). Written, not compiled. The `MM6CampaignChain` helper and C0 are not started. The MM6 profile prefix must be `mm6-restart-chain-`, because the MM6 test profile hook only accepts `mm6-restart-`. Next: build and fix, the helper, C0, then the runner checks with MM7's chain still passing.
- **X, MM6 house exits and NPC 12 rooms**, `claude/mm6-house-exits` in `mm7-load-admission`, `e98e66fe0` (research only, with a handoff on the branch). From the executable: house table columns 20-22 hold the exit picture, map and restriction. A positive restriction is a quest bit that must be set, and the inherited OpenEnroth test is inverted. A negative restriction picks an arrival from a table. The move autosaves. NPC 12's rooms run 168 to 553 to 554 through a movie when a character carries item 461. Lead decision: the confirmation buttons follow the original's y = 313 row. Next: `MM6HouseExit.h`, keep columns 20-22 in `MM6HouseServiceData`, the portrait and confirmation view, the room chain, tests.

## Next steps

1. Owner: the recorder and upstream-button playtest from `PLAN.md` (grant Screen Recording, about three minutes of play, Command-Shift-R, then Play upstream OpenEnroth).
2. Resume the three paused lanes, review, integrate one at a time with the M7 controls between.
3. Then the M4 lanes in `PLAN.md`: SpeakNPC dialogue (D), Question by input (Q), the chain chapters C1-C9, flight and water walking later.
4. Push `master` when the owner asks. Nothing has been pushed this session.

## Worktrees and disk

Ten of ten registered worktrees. Holding paused lanes: `mm7-save-durability` (F), `mm7-monster-combat` (H), `mm7-load-admission` (X). Free for reuse: `mm7-death-regression` and `mm7-mod-access` (no build), and the detached `mm7-save-atomic` (the owner can remove it with `git worktree remove --force`, which the auto-mode classifier blocked). The ccache is capped at 2 GB. Disk was 21 GiB free at handoff.

Privacy: no game data, saves, captures or logs in Git.
