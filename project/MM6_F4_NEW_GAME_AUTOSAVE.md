# MM6 first-map `.ec6` autosave boundary — F4

Status: **offline known-field candidate and native atomic disk path; live new-game SaveLoad guard closed** (2026-11-06).

## Why the first map is blocked

`Game::loop()` completes the MM6 party-creation UI, sets a pending `oute3.odm` transition, then calls `saveNewGame()` **before** `gameLoop()` can load the first map. `saveNewGame()` rejects MM6 at its first statement, so the later New Sorpigal position/map assignment and the inherited `saveGame(true, true, ...)` are unreachable. The generic `saveGame()` and `createSaveData()` also reject MM6 before inherited MM7 `.mm7` serialization. Removing only the top guard would encounter those later guards, and removing all guards would write the wrong title format.

## Bounded offline route

`encodeOfflineNewGameAutosave()` accepts an already detached `KnownSaveFields` seed, the verified 67-name MM6 manifest, an injected reader for default `.dlv`/`.ddm` entries from a trusted MM6 Games archive, a trusted base-map reader and loaded item-template verifier. It requires explicit MM6 title and `oute3.odm` HEAD before archive reads. It replaces any stale MAPS seed with exactly the 67 default entries, derives the CHST raw sidecar from the decoded first-map default delta, encodes the current seventeen-section `.ec6` candidate, then validates every default delta against its corresponding MM6 base and item references. No map, Party, SaveLoad or global is assigned. The caller must prepare the seed through represented Party/NPC/visual capture adapters; this function does not claim that capture is complete.

`writeOfflineNewGameAutosave()` performs that full **known-field** validation before calling the existing native atomic writer with fixed `autosave.ec6` under an explicit absolute MM6 profile root. The writer creates and syncs a temporary file, validates readback, and replaces the primary with backup handling. No `.mm7` slot, user filesystem abstraction or MM7 snapshot is used. This is an offline/test API and is not called by `saveNewGame()`.

Three focused native tests passed in a manually linked arm64 binary using the integrated v4 build libraries: an MM6 first-map autosave writes all 67 synthetic defaults and semantically preflights unchanged; MM7 title and wrong first map reject before delta reads or disk mutation; a malformed final default delta rejects after all 67 reads while preserving the previous autosave; and a corrupt primary selects a valid backup without mutation, then recovers the prior bytes. The existing atomic writer's fault-injection suite covers replacement failures separately. No proprietary game assets or MM6 process were opened.

## Before the live route can open

The seed still omits MM6-keyed SHOP/guild/bounty state, active beacon images/map identity, some movement/flight/fall and action state, generated-artifact identity, previous face/voice and other fields in the [save transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md). In particular, `saveNewGame()` sets `uFallStartZ = 340` while current `PART` stores only the position, so a faithful first-map reload cannot yet be claimed. The detached active-map candidate is not an instantiated world; loaded HEAD/resource identity, PID rebinding and no-throw all-owner commit are open. A real route must capture every mandatory owner, write and recover the native `.ec6` slot, then quit/relaunch/load into the correct rendered map without touching MM7. Keep `saveNewGame()`, `saveGame()`, `createSaveData()` and `loadGame()` guarded for MM6 until that proof exists.
