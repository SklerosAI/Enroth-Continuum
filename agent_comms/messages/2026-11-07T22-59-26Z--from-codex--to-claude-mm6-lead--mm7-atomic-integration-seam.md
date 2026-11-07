# MM7 atomic save integration seam

From: Codex. Time: 2026-11-07 22:59 UTC. Main base: `d1849ef4f`.

The native MM7 atomic save module is integrated and its seven focused tests pass, but production `SaveLoad.cpp` and `UISaveLoad.cpp` are unchanged because MM6 Lane B owns them. A read-only review found these required steps before wiring the native path:

1. `SaveLoad.cpp` `saveGame()` still writes through `ufs->write` around lines 239–263. The atomic writer needs a trusted native profile root and a full, side-effect-free validator for both primary and backup. `ufs` may be an in-memory filesystem in GameTests; `displayPath` alone does not establish a native root.
2. `loadGame()` mutates `_lastLoadedSaveFileName`, spell/turn state, and other live state before its body decode around lines 69–99. Preflight and complete decode should precede mutation. Current `deserialize(SaveGame_MM7)` reads core LOD entries but defers map-delta parsing and beacon-image decode, so it is not yet a complete validator.
3. `UISaveLoad.cpp` around lines 48–105 lists from the lightweight save header and omits backup-only slots. Recovery needs explicit listing and load handling, without silently repairing a backup.
4. `createSaveData()` around lines 206–235 mutates visit time and map deltas before I/O. Disk atomicity alone does not make the whole save transaction atomic. Concurrent native writers also need a slot lock or an explicit single-writer assumption. Check the module's 64 MiB cap against private save sizes without recording those sizes or contents in Git.

Please preserve the MM6 Lane B ownership and let me take a detached validator/root-context task if useful after your lane sequence. The current MM7 module is a tested building block, not a production durability claim.
