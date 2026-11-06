---
type: synthesis
up: "[[MM6-F4-Save-Remainder]]"
tags: [mm6, save, inventory, held-item]
---

# MM6 F4 Held Item

This page records the standalone `.ec6` field for the item held on the cursor and its restoration boundary.

`Party::pPickedItem` is outside character inventory `ITEM`. Character, chest, shop, NPC and popup actions read or transfer it. The required `HELD` v1 section stores an empty marker or every current `Item` scalar: ID, potion/gold values, enchantments, charges, raw flags, lich-jar index and expiry. It rejects unsupported IDs and a nondefault MM7 lich-jar index. A present record requires a loaded MM6 item template with a usable icon footprint before staging.

Capture and copy-staging require the active MM6 title. Staging changes only `pPickedItem` on a party copy. It never calls `Party::setHoldingItem`, which would place/drop an old item and mutate the global mouse. The full transaction must first commit the staged party; afterward its UI step should set the icon and recenter the cursor. The current click offset is transient. The required-section assembler and SaveLoad have not been changed.

Two standalone C++ tests pass empty/present round-trip and malformed/wrong-title no-mutation checks. The retained F3-matched item-footprint result supplies the bounded item-table expectation; no original installation was reread or MM6 game launched. See the [implementation packet](../project/MM6_HELD_ITEM_F4.md) for all fields, limits, consumers and F5 checks.

See also: [Save Remainder](MM6-F4-Save-Remainder.md), [Inventory Restore](../project/MM6_F4_INVENTORY_RESTORE.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
