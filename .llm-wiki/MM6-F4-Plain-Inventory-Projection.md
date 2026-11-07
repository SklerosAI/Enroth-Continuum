---
type: synthesis
up: "[[MM6-F4-Raw-Inventory-Section]]"
related: ["[[MM6-F4-Plain-Item-Projection]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, inventory, runtime, save]
---

# MM6 F4 Plain Inventory Projection

The [implementation packet](../project/MM6_F4_PLAIN_INVENTORY_PROJECTION.md) records an opt-in runtime view of the 138-slot physical MM6 character inventory for source-typed plain items and gold.

The stage validates every referenced raw item, unique in-range grid/equipment index and exact rebuilt grid and equipment words before returning a 138-slot `CharacterInventory`. It retains unreferenced records and all raw item tails. Capture requires the original active set, template type, zone and equipment slot, permits a backpack anchor move or the plain-item adapter's bounded field updates, and rejects item creation, removal, equipment changes or a changed template type. Its original raw source and placement metadata cannot be edited through the public runtime inventory accessor.

Linked focused tests passed 8/8, including record 138, negative continuation, equipment and malformed-reference cases. A monitored MM6 headless test with the loaded private-backed item table and real icon footprint passed 1/1 over 75 samples without an on-screen window or focus change. No Party owner, four-character transaction, normal `.ec6` manifest or live SaveLoad uses this projection yet; category-specific items and transfer semantics remain open.

See also: [Raw Inventory Section](MM6-F4-Raw-Inventory-Section.md), [Plain Item Projection](MM6-F4-Plain-Item-Projection.md), [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
