---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-Partial-Stage-Coordinator]]"]
tags: [mm6, f4, save, autosave, new-game]
---

# MM6 F4 New Game Autosave

The [first-map packet](../project/MM6_F4_NEW_GAME_AUTOSAVE.md) traces why `Game::loop()` reaches the MM6 `saveNewGame()` guard before its pending `oute3.odm` map can load, and records the offline `.ec6` autosave seam.

`encodeOfflineNewGameAutosave()` replaces MAPS in an already detached known-field seed with 67 title-specific default archive deltas, derives the active CHST raw sidecar from the decoded first-map default delta, then validates the eighteen-section candidate and all 67 base/delta pairs before returning bytes. `writeOfflineNewGameAutosave()` passes those bytes to the native atomic writer at fixed `autosave.ec6` in an explicit MM6 profile. Three synthetic native tests cover nonzero first-map state, wrong-title and wrong-map rejection before reads, malformed final-delta failure before disk mutation, and semantic backup preflight/recovery. The route is offline only; `SaveLoad.cpp` still rejects MM6 before MM7 serialization.

The seed is only the represented field set; the caller must prove and supply the MM6 chest LCG/artifact seed before a live route can use it. Active world reconstruction, native resource identity, MM6-keyed commerce/bounty, beacon images/map identity, first-frame movement/action policy and the final all-owner reload are not proved. `PART` v2 preserves fall-start Z; the offline builder now requires the source-backed 340 position and fall-start Z before archive reads. Keep live first-map autosave guarded until a full `.ec6` capture, atomic write/recovery, quit/relaunch/load and rendered-state scenario succeeds.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [Partial Stage Coordinator](MM6-F4-Partial-Stage-Coordinator.md), [Atomic Save File](MM6-F4-Atomic-Save-File.md).
