---
type: analysis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
derived_from: ["[[MM6-F4-Shop-Stock-Tuple-and-Autosave]]", "[[MM6-F4-Shop-Owner-Field-Gate]]"]
tags: [mm6, f4, save, shop, stock]
---

# MM6 F4 Shop Purchase-Write Gate

This [source audit](../project/MM6_F4_SHOP_PURCHASE_WRITE_GATE.md) separates proved physical stock-record identity from unproved purchase mutation and live ownership.

## Question

Can the existing MM6 physical stock index support a complete detached `.ec6` stock-record codec now?

## Context

The original executable identifies two shop banks and a guild bank with 12 positions per row, a 28-byte record stride, zero-ID display suppression, and a final-shop/first-guild physical alias. The [pure index helper](../src/Engine/MM6/MM6ShopStockIdentity.h) retains that alias.

## Analysis

The selected shop slot reaches an item display call, but the inspected direct references do not identify the purchase write, its 28-byte effect, or the next read. The pinned research editor models two 47-row shop lists and then a separate 22-row guild list. Its guild list starts one 12-slot physical row after the original first active guild address, so the editor cannot settle the original shared-edge policy. Its item-field labels are hypotheses until matched to original mutation and a nonzero native save. The fork also lacks an MM6-keyed live stock owner that could supply post-purchase bytes.

## Conclusion

Do not add a detached `SHOP` record codec yet. An opaque 28-byte copy proves only copying; an ID-only field loses data; disjoint arrays split the physical alias. The existing `SCLK` refresh-deadline section and normal MM6 SaveLoad guard remain as they are.

## Open follow-ups

Trace purchase writes and subsequent reads in all three bank paths, inspect a nonzero owner-local native save at the shared edge, then define one MM6-keyed physical owner and nonzero/duplicate/late-failure tests. Keep bounty and ban state outside this stock contract.

See also: [Shop Stock Tuple and Autosave](MM6-F4-Shop-Stock-Tuple-and-Autosave.md), [Shop Owner Field Gate](MM6-F4-Shop-Owner-Field-Gate.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
