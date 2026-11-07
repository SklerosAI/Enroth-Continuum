---
type: decision
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Item-Record-Conversion-Gate]]", "[[MM6-F4-Chest-Runtime-Projection]]"]
decided_at: 2026-11-06
tags: [mm6, f4, item, save, schema]
---

# MM6 F4 Raw Item Authority

The [implementation packet](../project/MM6_F4_RAW_ITEM_AUTHORITY.md) sets a byte-preserving MM6 item direction for the fork's unfinished save and runtime adapters.

## Question

Which representation should own MM6 item bytes when the inherited runtime uses a different 36-byte MM7 snapshot?

## Options considered

- Keep `Item_MM7` and the provisional logical `ITEM`/`HELD` v1 fields as the only owners. Existing copy-stage fixtures work, but MM6 bytes 26–27 have no place and MM7 adds an expiration field.
- Keep the source-proven 28-byte item record as a title-local raw owner, with context-specific runtime views. This preserves unknown bytes but requires explicit adapters and later save-schema revisions.

## Decision

Use `ItemRecord28` as the exact byte value for proven 28-byte contexts. Detached SHOP stock now uses it; chest records already retain full raw bytes in a sidecar. A layout-backed [detached INVR section](MM6-F4-Raw-Inventory-Section.md) retains all 138 physical character records and raw index tables. The accepted original executable confirms a 138-slot free-item path, and MM6 Party zeroing now allocates that capacity without changing MM7's 126-slot default. A [detached plain-item projection](MM6-F4-Plain-Item-Projection.md) uses MM6 table types and rejects unmapped bonus unions while retaining the raw source; a separate [standard-item projection](MM6-F4-Standard-Item-Projection.md) handles bounded equipment bonuses. Runtime raw-byte ownership and mutation rules remain unproved. Cursor and ground use require separate evidence; live SaveLoad stays guarded.

## Rejected alternatives

Do not infer original MM6 byte 26 from MM7's lich-jar index or synthesize MM7 expiration as native MM6 state. Current logical `ITEM` and `HELD` v1 are provisional current-runtime snapshots, not sufficient native-byte authorities.

## Revisit triggers

Revisit the ownership split if original inventory/held/ground evidence shows a different record size or lifecycle, or if a checked full conversion proves that a raw sidecar is unnecessary for a specific category. Any final manifest change needs nonzero round trips and before-mutation rejection tests.

See also: [Shop Item Conversion Gate](MM6-F4-Shop-Item-Record-Conversion-Gate.md), [Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md), [Raw Inventory Section](MM6-F4-Raw-Inventory-Section.md), [Plain Item Projection](MM6-F4-Plain-Item-Projection.md), [Standard Bonus Map](MM6-F4-Standard-Bonus-Map.md), [Standard Item Projection](MM6-F4-Standard-Item-Projection.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
