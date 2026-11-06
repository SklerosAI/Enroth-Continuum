---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, chest, ui, save]
---

# MM6 F4 Chest Interaction Gate

The [source audit](../project/MM6_F4_CHEST_INTERACTION_GATE.md) lists the consumers that follow chest item generation and placement before a player can use a chest.

`Chest::open` can call inherited placement and trap/disarm effects; `GUIWindow_Chest` uses a hardcoded 9×9 grid, which an accepted original-MM6 trace also finds in MM6 placement and UI despite different `dchest.bin` metadata. Its icon selector and other chest rules still need title-specific validation. Click and Grab Item mutate items, flags, held cursor and party gold. A valid generated loot result alone does not establish these operations. Active-map changed-state encoding and the complete `.ec6` transaction must preserve a nonzero chest across map leave, save and reload. The invalid chest-ID check was corrected to reject IDs outside the actual runtime chest vector before indexing.

The [detached placement](MM6-F4-Chest-Placement.md), [caller](MM6-F4-Chest-Generation.md), [bounded runtime projection](MM6-F4-Chest-Runtime-Projection.md) and [live-owner adapter](MM6-F4-Chest-Live-Owner-Adapter.md) pages carry proved subsets. Delta writers can encode a synthetic nonzero chest grid, but trap behavior, interaction cues, title-specific UI and durable persistence remain open. No MM6 chest has been opened on screen.

An opt-in [first-open transaction](../project/MM6_F4_CHEST_INTERACTION_GATE.md) now stages original 9×9 placement plus one visible-grid pickup on copied record, RNG, held-item and party-gold values. Ordinary item pickup moves an `Item` into the held slot; gold pickup requires an explicit net credit because original MM6 share/hireling effects remain unproved. Trapped chests, occupied cursor, bad grid references, malformed wire and MM7 reject with no owner mutation. An initialized chest reopens without a second placement draw. The helper does not commit `vChests`, the map sidecar or `.ec6`, and does not reproduce unproved trap, cursor-offset or hidden-item behavior. Focused native tests passed 3/3; the live chest guard remains closed.

A targeted read of retained original-PE chest entry confirms a different MM6 trap decision from the inherited MM7 threshold. Placement precedes active-character/trap checks; a nonzero disarm score draws `LCG % 10`, and strict `score + roll > 5 × map difficulty` disarms. Zero score skips that draw. A trigger draws `LCG % 4` for a variant, clears the trap bit after its effect calls and does not open the UI. `stageChestTrapDecision` models only this detached branch and returns no changed chest, damage or UI state; 3/3 seeded native tests passed. The original score helper's full owner join and trap effects remain open. The retained manual's broad chance/telekinesis description and guide gold-bonus notes do not establish damage or net hireling-adjusted chest gold, so the explicit net-credit requirement remains.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Agent Handoff](Agent-Handoff.md).
