---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Raw-Inventory-Section]]", "[[MM6-F4-Potion-Matrix]]"]
tags: [mm6, f4, item, runtime, save]
---

# MM6 F4 Plain Item Projection

The [implementation packet](../project/MM6_F4_PLAIN_ITEM_PROJECTION.md) records a narrow detached view of source-typed MM6 plain items and gold while the exact 28-byte record remains authoritative.

The F3-matched MM6 item table classifies some IDs in the inherited MM7 potion range as spell scrolls. The inherited `Item_MM7` snapshot uses ID ranges for its bonus union, so a byte round trip does not alone prove the right runtime item category. `stagePlainItem` requires an explicit MM6 title, loaded common-rarity template type and no bonus or bonus-strength fields; second bonus is accepted only for gold value. Non-common templates reject because their artifact/relic effects are outside this view. `capturePlainItem` permits only identified/broken flag changes, wand charges and maximum charges, or gold-value updates while preserving source body location and unknown tail bytes. It rejects unsupported logical fields and revalidates the source record.

Synthetic linked raw-item/inventory/projection checks passed 6/6. A monitored MM6 headless check with the loaded source-backed item table passed 1/1 for scroll, bottle and gold category selection with no visible window or focus change. The adapter has no live item owner or save route. MM6 enchanted items, potion power, equipment movement and item transfers still need original behavior and a raw owner that travels with the active item.

The [value provenance carrier](MM6-F4-Item-Value-Provenance.md) now travels with staged plain/gold `Item` copies and is required by capture. This does not open live ownership or the full save route.

The later rarity guard passed linked focused checks 13/13 and two loaded-table MM6 headless checks over 76 monitor samples with no on-screen window or focus change.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Item Value Provenance](MM6-F4-Item-Value-Provenance.md), [Raw Inventory Section](MM6-F4-Raw-Inventory-Section.md), [Bounded Inventory Projection](MM6-F4-Bounded-Inventory-Projection.md), [Standard Bonus Map](MM6-F4-Standard-Bonus-Map.md), [Potion Matrix](MM6-F4-Potion-Matrix.md), [Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md).
