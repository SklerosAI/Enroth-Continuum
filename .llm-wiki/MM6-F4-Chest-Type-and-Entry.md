---
type: synthesis
up: "[[MM6-F4-Chest-Placement]]"
related: ["[[MM6-F4-Chest-Generation]]", "[[MM6-F4-Map-Save-Fields]]"]
tags: [mm6, f4, chest, icon, placement]
---

# MM6 F4 Chest Type and Entry

The [source packet](../project/MM6_F4_CHEST_TYPE_ENTRY.md) reconciles the original MM6 chest table and executable. Retained `dchest.bin` has eight rows with 14×10 metadata and one-based icon IDs. Original placement and display instead use hardcoded **9×9** dimensions for all eight types and type-specific pixel offsets; the UI formats the icon name from the table ID. The original chest-open path places uninitialized items before active-character and trap checks.

Original placement thus scans **81** item slots, although the in-memory chest stores 140 fixed items and 140 signed grid cells. All 1,340 retained chest records use valid types, their nonempty items occupy slots below 81, and their grid cells are zero. Changed-state anchors, unplaced items and save/reload remain unproved. The inherited chest snapshot's area-based anchor bound happens to be 80 at 9×9, but that does not validate an MM6 load transaction.

The detached [`chestTypeLayout`](../src/Engine/MM6/MM6ChestTypeLayout.h) validates a caller-supplied complete table and returns original runtime size, offset and icon identity for MM6 only. Three linked native unit cases, retained-table native smoke and an ignored 67-map executable/table checker passed. No live caller, MM6 launch or full CMake build occurred. The chest generation guard remains closed.

See also: [Chest Placement](MM6-F4-Chest-Placement.md), [Chest Generation](MM6-F4-Chest-Generation.md), [Map Save Fields](MM6-F4-Map-Save-Fields.md).
