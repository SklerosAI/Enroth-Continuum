---
type: analysis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
derived_from: ["[[MM6-F4-Shop-Stock-Tuple-and-Autosave]]", "[[MM6-F4-Shop-Owner-Field-Gate]]"]
tags: [mm6, f4, save, shop, stock]
---

# MM6 F4 Shop Purchase-Write Gate

This [source audit](../project/MM6_F4_SHOP_PURCHASE_WRITE_GATE.md) records the original physical stock identity and purchase mutation while keeping live ownership and save persistence gated.

## Question

What can the original MM6 stock identity and purchase write support safely now?

## Context

The original executable identifies two shop banks and a guild bank with 12 positions per row, a 28-byte record stride, zero-ID display suppression, and a final-shop/first-guild physical alias. The [pure index helper](../src/Engine/MM6/MM6ShopStockIdentity.h) retains that alias.

## Analysis

The original successful purchase handlers for both shop banks and the guild copy a complete 28-byte record to inventory, then clear source bytes 0–24 while retaining bytes 25–27. The first shop bank joins the second bank's success block after computing its own selected-record address. Failed gold or inventory checks do not reach that source clear on the inspected paths. The pinned research editor models two 47-row shop lists and then a separate 22-row guild list. Its guild list starts one 12-slot physical row after the original first active guild address, so the editor cannot settle the original shared-edge policy. Its item-field labels remain hypotheses without a nonzero native save. The fork also lacks an MM6-keyed live stock owner that could supply post-purchase bytes.

The original new-game initializer clears complete 28-byte records in three contiguous runs of 564, 564 and 312 slots. These cover physical rows 0–46, 47–93 and 94–119. The last active shop row's second bank and the first active guild row both address physical row 94, which is cleared in the guild run. The later [shared-edge trace](MM6-F4-Shop-Shared-Edge-Refresh.md) proves the observed menu-level deadline arbitration; other writers and played-session presentation remain open.

A later [detached bounded purchase transaction](../src/Engine/MM6/MM6ShopBoundedPurchase.h) stages a plain or common standard item from one physical stock record and inserts it into a copied MM6 backpack. It publishes the backpack and partial-clear stock record together only after successful insertion, preserving the whole pre-sale record in `Item::mm6RawRecord` and source bytes 25–27. Unsupported rarity, category unions, bad footprint, full backpack, wrong title/table and nonzero stock body location reject without mutation. The owner and paid-house lifecycle remain outside this helper.

## Conclusion

A [detached MM6 helper](../src/Engine/MM6/MM6ShopStockPurchase.h) models the proved full-record copy and partial source clear. A raw [physical stock owner](../src/Engine/MM6/MM6ShopStockFields.h) and isolated `SHOP` v1 section preserve all 1,440 records without splitting the alias. Their earlier integrated focused unit suites passed **6/6** together and the private-backed MM6/LOD gate **331/331**. The newer bounded item handoff has a syntax-checked synthetic unit translation but no linked run yet. The required known-save manifest still rejects `SHOP`: no live MM6 service writer or owner lifecycle exists, and an ID-only field would lose data. The existing `SCLK` refresh-deadline section and normal MM6 SaveLoad guard remain as they are.

## Open follow-ups

Run the new bounded-purchase linked tests, trace any other writers/readers at the shared edge, inspect a nonzero owner-local native save, then define the live MM6-keyed owner lifecycle and nonzero/late-failure tests across actual services and disk publication. Keep bounty and ban state outside this stock contract.

See also: [Shop Stock Tuple and Autosave](MM6-F4-Shop-Stock-Tuple-and-Autosave.md), [Shop Shared-Edge Refresh](MM6-F4-Shop-Shared-Edge-Refresh.md), [Shop Owner Field Gate](MM6-F4-Shop-Owner-Field-Gate.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
