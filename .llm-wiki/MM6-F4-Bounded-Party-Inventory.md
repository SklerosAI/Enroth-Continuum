---
type: synthesis
up: "[[MM6-F4-Raw-Inventory-Section]]"
related: ["[[MM6-F4-Bounded-Inventory-Projection]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, inventory, transaction]
---

# MM6 F4 Bounded Party Inventory

The [implementation packet](../project/MM6_F4_BOUNDED_PARTY_INVENTORY.md) records a detached all-four-character inventory stage and capture transaction over `RawInventoryFields`.

Each character must satisfy the [bounded inventory projection](MM6-F4-Bounded-Inventory-Projection.md). Stage rejects a malformed late character without touching the source; capture returns a complete new raw owner only after all four recapture. Focused linked checks passed 11/11, including final-slot standard armor, opaque tail preservation, `INVR` codec round trip and late rejection. The live `Inventory::take`/`addAt`/`equipAt` paths copy `Item` values across slots, so a Party-slot-only raw sidecar would lose item provenance. A private local source check found no authentic nonzero MM6 save sample; validation still uses synthetic item records. This stage does not attach raw bytes to live `Party`, revise the final schema or open normal MM6 SaveLoad.

The [item value carrier](MM6-F4-Item-Value-Provenance.md) now retains raw source bytes across ordinary shared `Item` copies, but this detached stage still rejects moved or removed records until physical slot mutation and complete owner semantics are defined.

See also: [Raw Inventory Section](MM6-F4-Raw-Inventory-Section.md), [Bounded Inventory Projection](MM6-F4-Bounded-Inventory-Projection.md), [Item Value Provenance](MM6-F4-Item-Value-Provenance.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
