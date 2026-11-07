# MM6 raw item transfer semantics — F4 source audit

Status, 2026-11-06: a read-only audit of the accepted F3-matched original executable establishes bounded physical-record behavior for selected backpack, cursor and shop paths. It does not prove ordinary UI pickup/equip, ground fallback or every item category. Private addresses, disassembly and game bytes remain ignored locally.

| Proved source path | Bounded physical effect |
| --- | --- |
| Backpack removal reached by the item-event Subtract path | Copy the selected item before clearing its first 25 of 28 bytes; retain its final three bytes and clear the occupied grid footprint. The source is a positive one-based grid reference. |
| Free backpack record selection and placement | Search all 138 physical records by zero item ID, then write an anchor/continuation grid footprint and destination item ID. A free record can retain nonzero trailing bytes. The helper alone does not prove a full 28-byte source copy. |
| Equipment placement helper | Write one-based equipment reference, item ID and body-location byte. This does not prove equipment take/unequip clear behavior. |
| Held-item setter and clear | New incoming cursor record is copied as all 28 bytes. An occupied cursor is relocated with a full raw copy to a selected character record before the old cursor clears its first 25 bytes, retaining the last three. The destination and ground-fallback policies need separate proof. |
| Shop purchase | Existing accepted trace copies all 28 bytes to the destination and clears source bytes 0–24, retaining its tail. |

The [value-carried raw record](MM6_F4_ITEM_VALUE_PROVENANCE.md) can preserve source bytes through shared `Item` copies, but it is not a physical mutation policy. A safe next transaction must stage all four 138-record arrays, grids, equipment tables and the cursor on deep copies, validate unique live ownership and destination capacity, apply only a proved writer, then publish all owners together. It must preserve unreferenced records, including zero-ID tails. `Inventory::take` clears its whole runtime record and cannot by itself implement the native raw clear. Keep ordinary UI pickup, equip/unequip, ground fallback, live SaveLoad and final `INVR`/`HELD` schema guarded until each path and late failure is tested.

See [raw item authority](MM6_F4_RAW_ITEM_AUTHORITY.md), [bounded party inventory](MM6_F4_BOUNDED_PARTY_INVENTORY.md), [shop purchase gate](MM6_F4_SHOP_PURCHASE_WRITE_GATE.md) and [save transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md).
