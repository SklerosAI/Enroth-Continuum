---
type: synthesis
up: "[[MM6-F4-Raw-Inventory-Section]]"
related: ["[[MM6-F4-Plain-Item-Projection]]", "[[MM6-F4-Standard-Item-Projection]]"]
tags: [mm6, f4, inventory, runtime, save]
---

# MM6 F4 Bounded Inventory Projection

The [current implementation packet](../project/MM6_F4_BOUNDED_INVENTORY_PROJECTION.md) records a detached 138-slot MM6 character inventory stage that accepts bounded plain/gold and standard-enchanted equipment records while retaining every raw source byte.

This supersedes the [plain-only inventory stage](MM6-F4-Plain-Inventory-Projection.md). Source-typed item variants are chosen by the raw bonus/strength fields. Every referenced record must stage, every grid and equipment reference must rebuild exactly, and unreferenced bytes remain in the sidecar. Capture permits a backpack move or bounded item-field edits, but rejects item creation/removal, equipment transfers, slot changes and changed template types.

Focused linked checks passed 7/7. Two monitored MM6 headless loaded-table checks passed 2/2 over 65 samples without an on-screen window or focus change, covering a final-slot scroll with its actual icon footprint and standard-bonus armor in its equipment slot. There is no live Party raw owner, native nonzero save sample, effect verification or normal MM6 SaveLoad route.

The [four-character wrapper](MM6-F4-Bounded-Party-Inventory.md) now stages and recaptures all four physical regions atomically while remaining detached from live `Party`.

See also: [Raw Inventory Section](MM6-F4-Raw-Inventory-Section.md), [Bounded Party Inventory](MM6-F4-Bounded-Party-Inventory.md), [Plain Item Projection](MM6-F4-Plain-Item-Projection.md), [Standard Item Projection](MM6-F4-Standard-Item-Projection.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
