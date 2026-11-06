---
type: implementation
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, houses, ui]
---

# MM6 F4 House Type Routing

The [F4 packet](../project/MM6_F4_HOUSE_TYPE_ROUTING.md) records the title-specific `2devents.txt` type parser. It classifies every label in the 557-row retained house report, including the 384 active rows that previously fell into the inherited mercenary guild category. Unknown MM6 labels now produce an invalid type, which currently opens the generic house window. MM7's parser fallback remains unchanged.

This only corrects the type boundary. MM6-specific house videos, NPC pictures, services, prices, travel and special event screens still need implementation and F5 observation. The exact row/event report remains in ignored local data.
