---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Raw-Inventory-Section]]", "[[MM6-F4-Potion-Matrix]]"]
tags: [mm6, f4, item, runtime, save]
---

# MM6 F4 Plain Item Projection

The [implementation packet](../project/MM6_F4_PLAIN_ITEM_PROJECTION.md) records a narrow detached view of source-typed MM6 plain items and gold while the exact 28-byte record remains authoritative.

The F3-matched MM6 item table classifies some IDs in the inherited MM7 potion range as spell scrolls. The inherited `Item_MM7` snapshot uses ID ranges for its bonus union, so a byte round trip does not alone prove the right runtime item category. `stagePlainItem` requires an explicit MM6 title and loaded template type; it accepts only records with no bonus or bonus-strength fields and no second bonus except gold value. `capturePlainItem` permits only identified/broken flag changes, wand charges and maximum charges, or gold-value updates while preserving source body location and unknown tail bytes. It rejects unsupported logical fields and revalidates the source record.

Synthetic linked raw-item/inventory/projection checks passed 6/6. A monitored MM6 headless check with the loaded source-backed item table passed 1/1 for scroll, bottle and gold category selection with no visible window or focus change. The adapter has no live item owner or save route. MM6 enchanted items, potion power, equipment movement and item transfers still need original behavior and a raw owner that travels with the active item.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Raw Inventory Section](MM6-F4-Raw-Inventory-Section.md), [Plain Inventory Projection](MM6-F4-Plain-Inventory-Projection.md), [Potion Matrix](MM6-F4-Potion-Matrix.md), [Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md).
