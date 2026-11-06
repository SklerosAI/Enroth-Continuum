---
up: "[[MM6-F4-Chest-Generation]]"
---

# MM6 F4 Chest Placement

The [public placement packet](../project/MM6_F4_CHEST_PLACEMENT.md) summarizes a bounded original-MM6 PE trace. The original in-memory 4,204-byte chest addresses the final 280 bytes as 140 signed 16-bit grid cells after 140 × 28-byte item slots. This supersedes the F2 ambiguity **for in-memory placement only**. All 1,340 retained baseline delta chests have zeroes in that region; a changed original save or save-path trace is needed to prove serialized-grid equivalence. Retained `dchest.bin` gives 14×10 dimensions, while inherited MM7 uses 9×9.

`mm6::placeChestItemsDetached` models first placement into a zero grid. It reproduces the masked-eight-bit RNG shuffle (including the zero sentinel quirk), icon-based rectangular fit, one-based item-slot anchor and negative anchor-cell backlinks, conditional zero-charge wand fallback, no-fit item preservation and initialized flag. It copies input, preflights malformed rows, and returns staged grid/items/RNG without mutating live state. Seeded native cases cover a mixed chest, no-fit wand, cell 128 in a 14×10 grid, and atomic rejection. An entry-side call remains unclassified. Original already-initialized/nonempty-grid replay is not modeled.

The live MM6 chest generation guard remains closed. Next: title-specific chest dimensions and icon lookup, join the detached item picker/finalizer to all 140 slots, prove changed-grid `.ec6` save/reload and map reset, and test a full chest and direct artifact path. Keep MM7's 9×9 placement untouched. Exact PE evidence is in ignored `local-game-data/private-wiki/mm6/F4_CHEST_PLACEMENT_2026-11-06.md` and its referenced bounded disassemblies.
