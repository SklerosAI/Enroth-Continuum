# MM6 raw cursor and shop process owner handoff

Status, 2026-11-06: title-local process owner and checked detached publication are implemented and covered by four linked synthetic tests. The child startup, New Game, SaveLoad and house UI paths are still guarded or unwired.

## Source-backed boundary

`RawHeldCursorOwner` retains the exact native 28-byte HELD record, including the three trailing bytes when its item ID is zero. `ShopTitleState` owns the physical 1,440-record SHOP image and 68 keyed SCLK deadline words. The final shop row's second stock bank and first guild row share the same physical twelve records and clock slot. See [cursor/shop join](MM6_F4_RAW_CURSOR_SHOP_HANDOFF.md) and [shop field gate](MM6_F4_SHOP_OWNER_FIELD_GATE.md).

`rawHeldCursorState(MM6)` is a process-local cursor owner that begins unready; the existing `shopTitleState()` is its shop partner. `resetRawCursorShopProcessOwners(MM6)` clears both to unready. Child startup and New Game must call that reset before seeding fresh shops or restoring an EC6 v2 snapshot. The current code provides the reset entry point but does not yet wire those lifecycle calls.

`publishRawCursorShopOwners` accepts explicit destinations, the untouched decoded v2 fields and a detached `StagedRawCursorShop`. It checks the exact HELD/SHOP/SCLK join, title, readiness, stock IDs and non-null destinations before assigning either owner. The assignments are statically required to be nonthrowing, so a failed check leaves both destinations unchanged. The caller must first validate the complete v2 file, map/base identities, item templates, and every other owner candidate. This two-owner helper is not a whole-session transaction and assumes single-threaded child publication.

`readShopStock`, `readGuildStock`, `readShopDeadline` and `readGuildDeadline` expose only bounded native records or raw deadline words from a ready MM6 shop owner. They preserve the physical shop/guild alias and reject foreign titles, invalid rows, partial readiness and out-of-range item IDs. They do not project an MM7 `Item` or assign a price.

## Remaining live gates

- Call the reset at MM6 child startup and New Game, then seed or restore both owners through the all-owner transaction. Do not give MM7 access to the MM6 process cursor.
- Prepare and publish the visible held-item pointer and icon with rollback. Occupied native cursor relocation and UI reset semantics are not established by the raw owner.
- Route MM6 house stock, refresh and purchase through these raw identities after price, deadline, item transfer and inventory semantics are independently checked. The current UI still uses inherited Party stock arrays.
- Verify save, quit, relaunch and load, plus graphical cursor and vendor behavior, before calling the campaign route ready. No private assets or save bytes are in this document.

Four arm64 linked synthetic tests cover publication, reset, late refusal without partial mutation, raw service aliases and title isolation. They do not establish in-game service behavior.
