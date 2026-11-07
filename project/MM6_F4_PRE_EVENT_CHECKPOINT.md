# MM6 test-only pre-event checkpoint

Status, 2026-11-07: test-only MM6 checkpoint in the real world preparation
route. It runs after map/actor rebuild and deterministic NPC-name setup,
before `_461103_load_level_sub()` generates NPC rows and before `onMapLoad()`
executes map events. A later [test-only v3 scope](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md)
can publish ACTR/GNPC/RNGS at guarded points; no production MM6 load arms it.

The one-shot checkpoint arms only an MM6 save-load child. A callback can
continue or explicitly abort to Main Menu; the abort is caught in
`Game::gameLoop()` before timer unpause or input, with progress/transition
cleanup. Unarmed and MM7 paths do not run it. Linked focused unit checks
passed **4/4**.

In an owner-only disposable profile, the native v2 starter writer passed
**1/1**, then a separate fresh headless child took real Main Menu QuickLoad
and reached the checkpoint **1/1**. The callback observed rebuilt outdoor
actors, a fresh zero generated-row cursor and idle action queue. It aborted
before NPC generation, map events or input, with the primary slot unchanged.
An MM7 headless control passed **1/1** after the shared Game/Engine change.
This is an order and abort control, not a v3 save/reload or playable game.

The test-only scope now implements the staged replay order and linked
synthetic checks. A v3 fresh child has not exercised it; all final owners
and resources must agree after world preparation before any first input.
