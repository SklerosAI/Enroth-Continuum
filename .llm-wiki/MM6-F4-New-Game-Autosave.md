---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-Partial-Stage-Coordinator]]"]
tags: [mm6, f4, save, autosave, new-game]
---

# MM6 F4 New Game Autosave

The [first-map packet](../project/MM6_F4_NEW_GAME_AUTOSAVE.md) traces why `Game::loop()` reaches the MM6 `saveNewGame()` guard before its pending `oute3.odm` map can load, and records the offline `.ec6` autosave seam.

`encodeOfflineNewGameAutosave()` replaces MAPS in an already detached known-field seed with 67 title-specific default archive deltas, derives the active CHST raw sidecar from the decoded first-map default delta, then validates the eighteen-section candidate and all 67 base/delta pairs before returning bytes. `writeOfflineNewGameAutosave()` passes those bytes to the native atomic writer at fixed `autosave.ec6` in an explicit MM6 profile. Three synthetic native tests cover nonzero first-map state, wrong-title and wrong-map rejection before reads, malformed final-delta failure before disk mutation, and semantic backup preflight/recovery. The route is offline only; `SaveLoad.cpp` still rejects MM6 before MM7 serialization.

The newer `prepareOfflineNewGameSession()` draws a port-specific chest seed once into a detached owner, builds the default-map candidate, and stages all represented Party/Character, world, visual, CHST and SCLK owners before returning bytes and owned copies. It requires fresh zero refresh clocks and no party-owned chest artifacts. `writeAndStageOfflineNewGameAutosave()` writes the fixed slot only after this preflight. The returned 67 map buffers are ready for a future `pMapDeltas` handoff, but neither helper assigns process globals. Synthetic tests include nonzero staged Party/NPC state and a final Party-adapter failure that preserves the old disk slot and live owners. The live guard remains closed.

`captureFirstMapKnownSeed()` now captures represented Party/NPC/timer/overlay fields with arrival placement on a detached Party copy. `writeAndPublishOfflineNewGameAutosave()` preflights, writes, then publishes the validated 67-map cache and CHST title owner through no-throw swaps. It retains the old values for `rollbackPublishedFirstMapCache()` if map entry fails. This handoff remains an offline helper; it has no Party/world commit or disk-slot rollback, and no MM6 SaveLoad call-site uses it yet.

The separate test-only `saveNewGameMM6ScopedForTest()` checks the current MM6 engine and uses the owner-local F3 manifest, loaded MM6 Games LOD and item table to call `runScopedFirstMapEntryForTest()`. That runner snapshots the prior autosave primary and backup, then preflights, writes and publishes the cache/CHST before a bounded callback sees the staged Party. A callback failure restores the prior cache/CHST and exact slot pair, including an absent prior slot. The callback must not mutate other process globals. Normal `saveNewGame()` and all MM6 SaveLoad guards remain closed; real `DoPrepareWorld` is not yet an atomic callback.

The monitored `GameTests_MM6Save.cpp` exercises this wrapper with loaded private MM6 resources and an ignored F3 map source named by `ENROTH_MM6_MAP_RESEARCH_ROOT`. It writes only into a unique temporary native profile, checks first-map staged Party/cache/CHST state, then tests exact rollback of primary, backup and process cache/title owners after a callback failure. It skips when the private map source is absent. This does not call the full outdoor world loader.

The linked monitored private test passed 1/1 in 21.0 seconds with no focus change or on-screen window across 1,566 samples. The ignored log is `local-game-data/mm6/research-2026-11-05/F4_MM6_SAVE_HEADLESS_TEST_PRIVATE.log`. Full outdoor entry, broader world rollback and normal SaveLoad remain guarded.

The [first-map world rollback audit](MM6-F4-First-Map-World-Rollback.md) classifies the outdoor and engine owners changed after the save callback. A bounded scratch owner now protects actor, sprite, chest and decoration collections under late synthetic failure; the outdoor object, Party, transition/events and resource/UI effects still need joined rollback before a full loader can be called atomically.

The seed is only the represented field set. The older byte-builder requires the caller to supply CHST state; the staged new-game helper draws one port-specific seed into a detached owner. Active world reconstruction, native resource identity, MM6-keyed commerce/bounty, beacon images/map identity, first-frame movement/action policy and the final all-owner reload are not proved. `PART` v2 preserves fall-start Z; the offline builder now requires the source-backed 340 position and fall-start Z before archive reads. Keep live first-map autosave guarded until a full `.ec6` capture, atomic write/recovery, quit/relaunch/load and rendered-state scenario succeeds.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [Partial Stage Coordinator](MM6-F4-Partial-Stage-Coordinator.md), [Atomic Save File](MM6-F4-Atomic-Save-File.md).
