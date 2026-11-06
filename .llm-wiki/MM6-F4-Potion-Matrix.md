---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Held-Item]]", "[[MM6-F4-Spell-Rules]]"]
tags: [mm6, f4, items, potions]
---

# MM6 F4 Potion Matrix

This page records the bounded MM6 `useitems.txt` lookup and its effect boundary.

The [implementation packet](../project/MM6_F4_POTION_MATRIX.md) describes the new title-specific `PotionCombinations` table. MM6 reads `useitems.txt` from `icons.lod`, applies the known pseudo-image decompression and validates a 29×29 symmetric matrix before assigning it to `ItemTable`. Numeric cells return a candidate product ID on the matrix axis; the five nonnumeric marker codes remain opaque. The retained F3-matched copy yields 841 cells: 50 numeric and 791 marker cells. The owner's guide has two numeric-output disagreements with that table; exact rows stay private.

This lookup does not make an MM6 potion mix playable. The inherited `UIPopup.cpp` path still uses MM7 potion ID ranges, matrix and Alchemy skill checks. Marker effects, numeric-result behavior, ingredient consumption, potion power, bottle return, black/rejuvenation effects, and save/reload need title-specific rules and F5 observations. No original installation was reread or partial MM6 game launched.

See also: [F4 Offline Seams](MM6-F4-Offline-Seams.md), [Held Item](MM6-F4-Held-Item.md), [Table Consumer Audit](../project/MM6_TABLE_CONSUMER_SPEC.md).
