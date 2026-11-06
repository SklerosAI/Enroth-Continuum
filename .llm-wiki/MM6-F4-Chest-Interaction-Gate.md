---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, chest, ui, save]
---

# MM6 F4 Chest Interaction Gate

The [source audit](../project/MM6_F4_CHEST_INTERACTION_GATE.md) lists the consumers that follow chest item generation and placement before a player can use a chest.

`Chest::open` can call inherited placement and trap/disarm effects; `GUIWindow_Chest` uses a hardcoded 9×9 grid, which an accepted original-MM6 trace also finds in MM6 placement and UI despite different `dchest.bin` metadata. Its icon selector and other chest rules still need title-specific validation. Click and Grab Item mutate items, flags, held cursor and party gold. A valid generated loot result alone does not establish these operations. Active-map changed-state encoding and the complete `.ec6` transaction must preserve a nonzero chest across map leave, save and reload. The invalid chest-ID check was corrected to reject IDs outside the actual runtime chest vector before indexing.

The [detached placement](MM6-F4-Chest-Placement.md), [caller](MM6-F4-Chest-Generation.md) and [bounded runtime projection](MM6-F4-Chest-Runtime-Projection.md) pages carry proved subsets. Delta writers can now encode a synthetic nonzero chest grid, but trap behavior, interaction cues, title-specific UI and durable persistence remain open. No MM6 chest has been opened on screen.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Agent Handoff](Agent-Handoff.md).
