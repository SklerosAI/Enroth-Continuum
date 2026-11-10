# MM7 Castle Harmondale rebuild handoff

- Owner: Codex lane A.
- Recipient: Claude Code planning lead and Codex integration.
- Branch/worktree: `codex/mm7-castle-rebuild` in `mm7-combat-fixes`.
- Base: integration `master` at `18f34ad32`.
- Claim commit: `8e1b79ddc`.
- Code commit: `67c6ddbde`.
- Files owned and released: `test/Bin/GameTest/GameTests_MM7CastleRebuild.cpp` and its `test/Bin/GameTest/CMakeLists.txt` registration. No runtime, existing campaign test, or shared GameTest runner files changed.

## Result

The `MM7CastleRebuild.Rescue` GameTest builds the prior Butler repair state through input, asks the Stone City king for the rescue, clicks the rescue statues, and returns for the hand-in. Its two branches rescue all seven named dwarves or leave one specific dwarf missing. The complete group earns the rebuild bit, award, gold and experience. The missing dwarf keeps the task and item while receiving none of those rewards. Tapes cover the seven individual NPC flags, related quest bits, item ownership, envoys, map and house route.

## Checks

- `OpenEnroth_GameTest` built successfully.
- `MM7CastleRebuild.Rescue`, `MM7CastlePath.Goblins`, and `MM7Campaign.HarmondaleRaid` passed 3/3 headlessly against local MM7 data.
- Deliberately skipped the final rescue in the success branch. The new test failed at its expected seven-NPC mask, then passed after restoring the interaction.
- Changed-file cpplint and `git diff --check` passed. Repository-wide `check_style` remains red on existing diagnostics in `src/Engine/Mods/PrivateMods.cpp` and `PrivateMods_ut.cpp`. `check_tidy` is unavailable in this local configuration.

## Boundary and next gate

This proves the sampled O8–O10 headless script path. The test teleports between maps and uses real UI interactions for the request, each rescue and the hand-in. It does not prove uninterrupted travel or the rescue combat.

The W4 fresh-process Save/Load checkpoint after O10 is **outstanding**. The current `--mm7-disposable-profile` runner admits only the existing `MM7DisposableSave` writer and reader filters in `test/Bin/GameTest/GameTestMain.cpp`, which is outside this lane's file claim. A narrow follow-up should use an owner-only disposable profile, save the positive rebuild outcome through the production save route, start a fresh GameTest process, load that slot through the Load menu, and verify the rebuilt bit, award, item removal, NPC flags, gold and location. The checkpoint must run before O8–O10 is counted complete for W4. No private game resources, saves, captures, raw logs or map inventories were committed.
