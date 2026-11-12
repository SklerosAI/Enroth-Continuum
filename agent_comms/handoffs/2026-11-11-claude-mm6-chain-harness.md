# MM6 campaign chain harness and C0 handoff

- Owner: Claude Code lane H, reviewed by the Claude lead. Recipient: the lead for integration, then the chapter lanes C1-C4 and C5-C9.
- Branch: `claude/mm6-chain-harness` in the `mm7-monster-combat` checkout, rebased onto `master` `6114f76e8`. Commits `44c13c3cc` (runner title switch, WIP title kept), `ef45939cb` (gate rows and `--mm6-chain-profile`, WIP title kept, compiles unchanged), `0c72c704d` (helper and C0), then this handoff. Not pushed.
- Files: `tools/run_mm7_campaign_chain.py` (title switch), `test/Bin/GameTest/GameTestMain.cpp` (MM6 gate rows), `GameTestOptions.{h,cpp}` (`--mm6-chain-profile`), new `MM6CampaignChain.{h,cpp}` and `GameTests_MM6CampaignChainC0.cpp`, CMake lines. No `src/` file changed.

## What works

- `tools/run_mm7_campaign_chain.py --title mm6` creates an owner-only `mm6-restart-chain-*` profile under the system temp root with the `.ec-mm6-campaign-chain` marker, runs `MM6CampaignChain.DISABLED_ChapterN` one process each with `--game mm6 --rules-path --mm6-chain-profile`, stops at the first failure, keeps each `saveNNN.ec6` with a digest pass marker, and resumes or removes the profile like the MM7 chain. Data and rules paths default to `MM6_DATA_PATH` and `MM6_RULES_PATH`. `--title mm7` is the default and unchanged.
- GameTest admits an MM6 chapter only with its exact filter, `--gtest_also_run_disabled_tests` and a validated profile. C0 needs a profile holding only the marker. CN needs `save{N-1}.ec6` and refuses its own and later slots, a quick save or any other entry. A gate filter without the profile, the profile with any other filter, or the option on MM7 fails before the engine starts. The validated root reaches the MM6 save code through `setMM6CampaignChainProfile`, which wraps `mm6::testing::setNativeProfileRootForTest`.
- `MM6CampaignChain.{h,cpp}`: `assertMM6CampaignChainProfile`, `loadMM6CampaignChainSlot` (main menu Load screen, scrolls past seven slots, checks the loaded file name and an armed session) and `saveMM6CampaignChainSlot` (Escape, Save menu, scrolls back to the new save slot, double-click, types `cN`, Save, then checks that the slot is the first free numbered slot `saveNNN.ec6`, lists with that name and the current map, and reads back as a loadable MM6 save).
- C0: New Game from the main menu, party creation by input (each character's first three optional skills and up to three points in every attribute), entry to `oute3`, then the Save menu save to `save000.ec6`. Tapes pin the map (`MAP_INVALID` then `oute3`), the first-map save arming, the "Game saved" status and the profile's saves folder (empty, then the new-game autosave, then the autosave and `save000.ec6`).

## Why C0 ends there

PLAN M4 names C0 "New Game and opening" and C1 "councils A and C", and gives the opening no quest step of its own. In the run the party arrives on `oute3` straight into the game view, with the new-game autosave written, and no dialogue or quest prompt comes up. The wiki's Quick Start audit only records an opening letter in the starting inventory. I did not scan the `oute3` event scripts for an opening step, so if the M4 read-only pass meant one, it belongs at the start of C1 or at the end of C0. The natural first save point is therefore the first-map save right after New Game, from the Save menu as a player would make it. It is the first-map v5 save that M1 already proved loads in a fresh process. A throwaway C1 (not committed) loaded it through the runner, walked, and saved `save001.ec6` as a session save, which ran the helper's load and scroll paths and the gate's CN rule end to end.

## Fixtures

- C0: none. No quest bit, NPC field, map or party write.
- Party strength: not applied. W4 put the single strength fixture in C0. If C1-C4 need one, add it at the end of C0 before the save, name it in C0's first comment, and rerun the chain from a fresh profile.

## Checks

All one process at a time, with the load gate the coordinator asked for.

- `check_style`: clean.
- Unit tests: 1,120 passed, 45 skipped.
- MM6 chain through the runner from a fresh profile, `--title mm6 --through 0`, twice: both passed (about 100 s each), the profile then held only its marker file, `autosave.ec6`, `save000.ec6` and the pass marker. Both profiles removed with `--remove-profile`.
- MM6 chain resume with the throwaway C1: `--profile <kept C0> --through 1` removed an unverified `save001.ec6`, ran C1 and passed. Gate refusals checked by hand: C0 on a used profile, C1 with its own slot present, a gate filter without the profile, the profile with a wildcard filter, the option on MM7.
- MM7 chain through the same runner, `--through 0` from a fresh profile: passed.
- MM6 suite `MM6*`: 115 passed, 2 skipped (the chain chapters are disabled and not counted).
- MM7 controls `MM7*:VendorRefreshIntegration.*`: 107 passed, 2 skipped.
- No file under the owner's `Profiles/` changed during the runs. Capture: `local-game-data/progress-captures/20261111-130811-mm6-chain-c0-oute3-oute3.png`.

## Adding C1

1. Add `GameTests_MM6CampaignChainC1.cpp` to `GAME_TEST_MAIN_SOURCES`. The gate rows for C0-C9 already exist.
2. Start with a first comment naming the fixtures, then `assertMM6CampaignChainProfile()`, `loadMM6CampaignChainSlot(game, 0)` and entry asserts (map `oute3`, the quest bits C1 expects unset). Create tapes, `test.startTaping()`, play by input, and end with `saveMM6CampaignChainSlot(game, 1)`. Never write quest bits, NPC fields or map state at the start.
3. Run `tools/run_mm7_campaign_chain.py --title mm6 --through 1`, and `--profile <path>` to resume from a kept C0.

## Open

- Each chapter process takes about 100-130 s on this machine even for C0, most of it outside the test body. Ten chapters will need about 20 minutes plus play time.
- The runner keeps the `run_mm7_campaign_chain.py` name although it runs both titles. Renaming it is the lead's call.
- MM6 save names typed in the Save menu are `c0` to `c9`, one digit per chapter, which fits the ten chapters.

Privacy: no game data, saves, captures or logs in Git.
