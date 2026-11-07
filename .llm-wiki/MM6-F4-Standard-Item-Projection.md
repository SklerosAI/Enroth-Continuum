---
type: synthesis
up: "[[MM6-F4-Standard-Bonus-Map]]"
related: ["[[MM6-F4-Plain-Item-Projection]]", "[[MM6-F4-Bounded-Inventory-Projection]]"]
tags: [mm6, f4, item, enchantment, runtime]
---

# MM6 F4 Standard Item Projection

The [implementation packet](../project/MM6_F4_STANDARD_ITEM_PROJECTION.md) records a detached raw-preserving MM6 view for standard-enchanted equipment.

The accepted original item-picker trace writes the one-based bonus index and positive strength for equipment template types 3–11. The adapter uses the shared [14-index MM6 map](MM6-F4-Standard-Bonus-Map.md), requires the loaded item table to classify the source as equipment and rejects a special-bonus word. Capture can change the mapped standard attribute, positive strength and identified/broken bits while retaining all other source bytes. It rejects changed IDs, types or unsupported logical fields.

Focused bonus-map and projection unit checks passed 3/3. A monitored headless MM6 check with a loaded armor template passed 1/1 over 65 samples without an on-screen window or focus change. The later [bounded inventory stage](MM6-F4-Bounded-Inventory-Projection.md) uses this view but does not attach it to Party, live equipment effects or normal SaveLoad. Native nonzero item evidence, modded armor behavior and save/reload remain open.

See also: [Standard Bonus Map](MM6-F4-Standard-Bonus-Map.md), [Plain Item Projection](MM6-F4-Plain-Item-Projection.md), [Bounded Inventory Projection](MM6-F4-Bounded-Inventory-Projection.md), [Raw Item Authority](MM6-F4-Raw-Item-Authority.md).
