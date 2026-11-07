---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Cursor-Shop-Handoff]]", "[[MM6-F4-Raw-Cursor-Owner]]", "[[MM6-F4-Shop-Owner-Field-Gate]]"]
tags: [mm6, f4, save, cursor, shop]
---

# MM6 F4 Raw Cursor and Shop Process Owner

The [implementation packet](../project/MM6_F4_RAW_CURSOR_SHOP_PROCESS_OWNER.md) describes a title-gated process cursor and checked HELD/SHOP/SCLK publication handoff. `rawHeldCursorState(MM6)` starts unready, while the existing title-local `shopTitleState()` is its partner. MM6 child startup and each New Game request now clear both; a later full-session transaction must seed or restore them before use.

`publishRawCursorShopOwners` verifies a detached pair against untouched decoded v2 fields before either destination changes. Assignments are nonthrowing. The four linked synthetic process-owner tests passed, including a late divergence that leaves both destinations intact; the focused raw-session, cursor/shop and process-owner run passed 21/21. The service readers return native 28-byte records and raw clock words, preserving the shared shop/guild physical edge and rejecting invalid title, row or readiness. They do not use MM7 `Item` projection.

The whole save/load transaction, occupied cursor transfer, visible icon and pointer, shop price and refresh rules, inventory transfer and live service UI remain open. Do not mistake this checked two-owner publication for live save/load admission or a playable shop.

See also: [Cursor and Shop Handoff](MM6-F4-Raw-Cursor-Shop-Handoff.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
