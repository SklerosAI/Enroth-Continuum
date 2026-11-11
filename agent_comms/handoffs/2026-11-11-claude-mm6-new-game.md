# MM6 production New Game handoff (gate row M1)

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm6-new-game` in the `mm7-monster-combat` checkout, from `master` `f0db53a79`. Commits `4d764f309` (session) and `f88d67f12` (post-world hook and gate group). Not pushed. The `codex/mm6-notes-book` ref is kept.

## Design

`saveNewGame()`'s MM6 branch calls `mm6::startNewGameSession()` instead of throwing. The new `src/Engine/MM6/MM6NewGameSession.{h,cpp}` does in memory what the seed and v4 gate children did on disk: native records for the creation items, a chest seed, a first-map v2 encode staged back through the v2 load check, and publication of the staged owners. A one-shot chest admission lets only the next first-map chest load use that state. `armNewGameSessionAtFirstMap()`, called from `Game::gameLoop` right after `releaseV5FirstMapLoadAttempt`, captures the v4 anchor after world preparation and before the first world update, then arms the Save menu writer. The capture can't happen later, because sprite and actor positions move off values the save sections store exactly. If the creation items can't be represented, the map still loads and the status bar says the party can't be saved.

Shared core touched: `src/Engine/SaveLoad.cpp` (MM6 branch only) and `src/Application/Game.cpp` (two MM6-gated lines and an include). Creation skill buttons gained names so tests can click them.

## Checks

- New `GameTests_MM6NewGame.cpp`: `NewPartyEntersFirstMap` in the normal suite, creation by input. Two disabled gate children: Save from the menu, then a fresh process loads, walks and saves with backup rotation.
- Revert-and-fail for both commits.
- Full MM6 first-map gate 27/27, including the MM7 controls. MM7 `MM7*:VendorRefreshIntegration.*` 107 passed, 2 skipped. Unit tests 1,087 passed. `check_style` clean.

## Integration order

Merge before `claude/mm6-reputation-var`, whose PART v4 bump changes the save this route writes. Rerun the new-game gate children after both.

## Open

The test profile root comes from a test hook because GameTest uses a RAM filesystem. The chest seed uses `random_device`, the existing policy. Visible F5a acceptance of a new game is still an owner session (PLAN M5).

Privacy: no game data, saves or logs in Git.
