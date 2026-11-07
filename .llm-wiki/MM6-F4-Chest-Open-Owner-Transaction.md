---
type: synthesis
up: "[[index_Enroth-Continuum]]"
extends: "[[MM6-F4-Chest-Interaction-Gate]]"
tags: [mm6, chest, transaction, campaign]
---

# MM6 F4 Chest Open Owner Transaction

The opt-in owner transaction joins one untrapped MM6 chest's first placement to its active map sidecar and title-local LCG.

`stageChestOpenOwner` decodes the current indoor or outdoor map delta, requires twenty raw records, and compares the selected live chest's header, supported item fields and full grid with the raw record. A world position is allowed and preserved. It calls the detached, source-backed `stageChestFirstOpenInteraction` in open-only mode, projects the changed record into a runtime chest, patches the one active sidecar record, and returns copies of the candidate owners. `applyChestOpenOwner` publishes the candidate with no-throw vector swaps and a scalar LCG assignment only after every stage succeeds. Reopening an initialized chest leaves the sidecar and LCG unchanged.

The path refuses a foreign title, absent or unready title state, malformed sidecar, stale runtime, invalid chest index, trap, failed icon size lookup, or failed projection without owner mutation. `Chest::open` now selects this MM6 path after validating the index. It builds the active outdoor/indoor binding, checks MM6 `dchest` and icon dimensions, stages the full first-open candidate, constructs the chest window before owner mutation, then publishes the chest vector, raw sidecar and title LCG together. It never calls inherited MM7 placement for MM6. Trapped chests remain closed, and MM6 click/space pickup returns without mutation while raw cursor/inventory/save ownership is unfinished. Normal MM6 map and SaveLoad guards still prevent an accepted playable route.

Linked arm64 synthetic tests passed 3/3 for outdoor first-open/reopen, indoor sidecar patching, position retention, raw sidecar/runtime agreement and rollback for foreign title, trap and stale runtime. A separate fresh headless GameTest loaded the real first outdoor map, opened one untrapped uninitialized chest through `Chest::open`, invoked its GUI update under NullRenderer, and verified the screen, pause, initialized chest and changed raw sidecar. It passed 1/1 with no visible window or focus change. The test closes its GUI before engine teardown. This does not prove rendered pixels, trap/pickup behavior, normal SaveLoad or campaign play.

See also: [Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [Chest Live Owner Adapter](MM6-F4-Chest-Live-Owner-Adapter.md), [Chest Map Load Route](MM6-F4-Chest-Map-Load-Route.md).
