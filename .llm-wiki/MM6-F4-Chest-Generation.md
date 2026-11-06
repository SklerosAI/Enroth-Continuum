---
up: "[[MM6-F4-Map-Save-Fields]]"
---

# MM6 F4 Chest Generation

The [public audit](../project/MM6_F4_CHEST_GENERATION_AUDIT.md) records the live-load chest gate. Retained F3-backed map data contain 943 random-item markers across six MM6 negative IDs; the save validator accepts them as references, not generated items. `Engine::_461103_load_level_sub()` now routes through a title guard: MM6 throws before inherited chest/RNG/artifact mutation, MM7 retains the existing `GenerateItemsInChest()` path. The inherited artifact picker points at MM6 message-scroll IDs. The MM6 chest grid/tail remains opaque. **Every MM6 level load reaching this call is blocked**, including marker-free maps, until a generator or proved skip policy exists.

No MM6 generation rule was inferred or implemented. Next agents should read the ignored `local-game-data/private-wiki/mm6/F4_CHEST_GENERATION_2026-11-06.md`, prove original MM6 roll and placement semantics against the accepted PE identity, then test all six markers, artifact state, full chests and save/reload before lifting the guard. MM7 must keep its own generator. Do not treat map-item validation or one successful chest opening as generation closure.
