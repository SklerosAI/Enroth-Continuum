# MM6 EC6 v2 cursor and shop owner handoff

Status, 2026-11-06: detached load-stage join, linked synthetic tests; no live publication or SaveLoad change.

## Source contract

The accepted original cursor trace copies an incoming 28-byte record and clears only bytes 0–24 when held item ID becomes zero. `HELD` v2 retains those final three bytes. The accepted shop trace uses one physical 1,440-record array of 28-byte stock entries; the final shop row's second bank and first guild row refer to the same twelve records. `SCLK` has 68 distinct 64-bit deadline words, with that same shop/guild edge sharing one clock slot. The tracked [raw cursor owner](MM6_F4_RAW_CURSOR_OWNER.md), [shop owner field gate](MM6_F4_SHOP_OWNER_FIELD_GATE.md), [stock identity](../src/Engine/MM6/MM6ShopStockIdentity.h), and [shared refresh edge](MM6_F4_SHOP_SHARED_EDGE_REFRESH.md) record the bounded evidence. Exact private executable bytes and owned game data remain ignored.

`stageRawCursorShop` accepts already decoded `RawSaveFields` from the EC6 v2 path and creates a fresh `RawHeldCursorOwner` plus a copied `ShopTitleState`. It requires MM6, a valid present or absent raw cursor, and both `SHOP` and `SCLK` ready. `validateRawCursorShopJoin` checks that the detached cursor captures back to the exact saved `HELD` field and that both shop owners match byte-for-byte, including zero-ID stock tails and every deadline. It reruns the three field encoders and rejects any nonzero shop item ID above the MM6 table's 580 maximum. A malformed final stock record, partially ready shop, bad absent cursor or divergent cursor/stock/clock stops before any live owner changes. The check does not infer item type or effect from a 28-byte record.

## Handoff to the all-owner transaction

1. Decode and validate the complete EC6 v2 manifest, active map bases and referenced item templates first. This helper is deliberately narrower than `stageRawTitleOwners`; a caller must not treat a manually constructed `RawSaveFields` as a verified file.
2. Stage Party/inventory, world, UI, resource and cursor-image candidates separately. Before publication, call `validateRawCursorShopJoin` against the untouched decoded save fields and the exact cursor/shop instances selected for commit. This equality gate applies to load snapshots; after legitimate in-game edits, capture a new raw save instead.
3. Establish a failure-atomic commit/rollback plan for the visible cursor, its icon, the raw cursor owner, and the title-local shop owner. `publishShopTitleState` can assign a ready shop copy without throwing, but this helper does not publish either owner or load an icon. Do not make a successful detached join a live save/load admission criterion by itself.
4. Exercise held-present and held-absent-tail saves, nonzero shared shop stock and deadline, an invalid late row, title isolation, then save–quit–relaunch–load with graphical cursor and vendor checks. MM7's existing owner remains separate.

Four focused arm64 linked synthetic tests passed for present and absent native cursor records, absent-tail and physical stock/clock edge preservation, exact cross-owner equality, late divergence, partial readiness, invalid last item ID and wrong title. No private item records or game files were committed.
