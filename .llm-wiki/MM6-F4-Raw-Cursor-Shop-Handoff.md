---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Cursor-Owner]]", "[[MM6-F4-Shop-Owner-Field-Gate]]", "[[MM6-F4-Raw-Save-Runtime]]"]
tags: [mm6, f4, save, cursor, shop]
---

# MM6 F4 Raw Cursor and Shop Handoff

The [implementation packet](../project/MM6_F4_RAW_CURSOR_SHOP_HANDOFF.md) defines one detached EC6 v2 join for `HELD`, physical `SHOP` and keyed `SCLK` before later live publication.

`stageRawCursorShop` creates fresh title-local cursor and shop owners from decoded raw fields. `validateRawCursorShopJoin` requires an exact cursor-to-HELD and shop-to-SHOP/SCLK match, ready sections, MM6 title and bounded shop item IDs. It retains the three opaque bytes after an empty cursor and the single physical stock/clock alias at the final-shop/first-guild edge. Four linked synthetic tests passed; a malformed last record and divergent or partially ready owner reject without changing the source fields.

This is an equality gate for a load snapshot. The complete v2 decoder, map/base and template verification, Party/world/UI stages, cursor image preparation and failure-atomic publication must precede a live route. Changed in-game owners require a new save capture. The helper does not publish the global shop state, write a slot, or change the MM6 SaveLoad guard.

See also: [Raw Cursor Owner](MM6-F4-Raw-Cursor-Owner.md), [Shop Owner Field Gate](MM6-F4-Shop-Owner-Field-Gate.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).

The subsequent [process-owner handoff](MM6-F4-Raw-Cursor-Shop-Process-Owner.md) adds a checked two-destination publication adapter and raw service readers. It does not wire the child lifecycle or house UI.
