---
type: decision
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Stock-Tuple-and-Autosave]]", "[[MM6-F4-Shop-Guild-Clock-Save]]"]
tags: [mm6, f4, save, shop, stock]
---

# MM6 F4 Shop Owner Field Gate

The [field-gate packet](../project/MM6_F4_SHOP_OWNER_FIELD_GATE.md) distinguishes the detached raw `SHOP` shape from a required live `.ec6` owner.

The only joined save field currently owned by MM6 shop/guild code is the MM6-row-keyed 64-bit refresh deadline, represented by required `SCLK` v1. The original stock paths prove two shop banks, one guild bank, 12 positions per row, a 28-byte item stride, a zero-ID display check and raw original Party-block persistence. The last shop row's second bank and first spell-guild row address the same 12 physical records. A title-explicit [stock identity helper](../src/Engine/MM6/MM6ShopStockIdentity.h) preserves that alias, and a detached [purchase helper](../src/Engine/MM6/MM6ShopStockPurchase.h) copies the complete 28-byte item and clears source bytes 0–24 as the original success paths do. Detached [stock fields](../src/Engine/MM6/MM6ShopStockFields.h) now hold all 1,440 physical records and round-trip them as isolated `SHOP` v1. Full item field semantics, shared-service lifetime and a nonzero native saved tuple remain unproved. The fork has no MM6-keyed live stock owner; inherited MM7 arrays cannot represent the MM6 guild and candidate bounty rows. The raw codec therefore does not define a complete updated/restored gameplay owner.

Decision: retain the existing required SCLK section and the coordinator's rejection of nonempty inherited commerce arrays. Keep detached `SHOP` outside the eighteen-section known-save manifest until the next read and family-edge behavior are verified against an owner-local nonzero save, then connect a title-local runtime owner and test live/disk late-failure atomicity. Bans and bounty need their own original row/lifetime evidence.

The ignored original-stock checker passed accepted executable identity, row/stride/boundary, raw save span and cross-map autosave assertions. Linked `MM6ShopGuildClockFields.*` passed 2/2. No proprietary data entered the repository, and the normal MM6 SaveLoad guard remains closed.

See also: [Shop Stock Tuple and Autosave](MM6-F4-Shop-Stock-Tuple-and-Autosave.md), [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Shop Guild Clock Save](MM6-F4-Shop-Guild-Clock-Save.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
