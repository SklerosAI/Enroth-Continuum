---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, chest, rng]
---

# MM6 F4 Chest Outer Plan

The [detached outer plan](../project/MM6_F4_CHEST_OUTER_PLAN.md) models source-order iteration over 20 chests and 140 item slots using the accepted original MM6 executable trace.

It consumes an entry probability draw even on a marker-free map, then preserves the first-attempt probability across markers until an additional attempt draws a replacement. It stages empty, MM6 gold and delegated ANY/direct-artifact results without touching live chest, RNG or artifact owners. Five seeded native tests pass, including multi-marker probability carry and full-capacity draw order. This is an item-ID and newly generated gold-amount plan; complete item fields, 9×9 placement, artifact policy, changed-grid save encoding and atomic live commit are separate gates.

See also: [MM6 F4 Chest Generation](MM6-F4-Chest-Generation.md), [MM6 F4 Chest Placement](MM6-F4-Chest-Placement.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md).
