---
type: decision
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Stock-Tuple-and-Autosave]]", "[[MM6-F4-Shop-Guild-Clock-Save]]", "[[MM6-F4-Shop-Shared-Edge-Refresh]]", "[[MM6-F4-Shop-Shared-Row-Access-Audit]]", "[[MM6-F4-Shop-Item-Record-Conversion-Gate]]"]
tags: [mm6, f4, save, shop, stock]
---

# MM6 F4 Shop Owner Field Gate

The [field-gate packet](../project/MM6_F4_SHOP_OWNER_FIELD_GATE.md) distinguishes the detached raw `SHOP` shape from a required live `.ec6` owner.

The only joined save field currently owned by MM6 shop/guild code is the MM6-row-keyed 64-bit refresh deadline, represented by required `SCLK` v1. The original stock paths prove two shop banks, one guild bank, 12 positions per row, a 28-byte item stride, a zero-ID display check and raw original Party-block persistence. The last shop row's second bank and first spell-guild row address the same 12 physical records. A title-explicit [stock identity helper](../src/Engine/MM6/MM6ShopStockIdentity.h) preserves that alias, and a detached [purchase helper](../src/Engine/MM6/MM6ShopStockPurchase.h) copies the complete 28-byte item and clears source bytes 0–24 as the original success paths do. Detached [stock fields](../src/Engine/MM6/MM6ShopStockFields.h) hold all 1,440 physical records and round-trip them as isolated `SHOP` v1. The [shared-edge trace](MM6-F4-Shop-Shared-Edge-Refresh.md) proves menu-level deadline arbitration, modeled by a detached [refresh seam](../src/Engine/MM6/MM6ShopRefreshBoundary.h). Full item field semantics, other writers and a nonzero native saved tuple remain unproved. Inherited MM7 arrays cannot represent the MM6 guild and candidate bounty rows.

The new [MM6 shop title state](../src/Engine/MM6/MM6ShopTitleState.h) supplies a process-local owner for the raw stock image and its 68 refresh deadlines. It starts unready, can form an explicit empty New Game seed, and stages `SHOP` plus `SCLK` from one validated `.ec6` container before a no-throw publication of both fields. A focused arm64 source translation check and linked standalone probe passed the shared shop/guild alias, nonzero record tail and deadline, wrong-title rejection, and malformed final `SHOP` or `SCLK` with the prior owner unchanged. This is the first process owner primitive; no production New Game, house generation, display, purchase, or load path calls it yet. The required known-save manifest still rejects `SHOP`, and the new owner does not settle the original indirect writer or item conversion gaps.

Decision: retain the existing required SCLK section and the coordinator's rejection of nonempty inherited commerce arrays. Keep detached `SHOP` outside the eighteen-section known-save manifest until a nonzero owner-local native save, the [indirect-access gap](MM6-F4-Shop-Shared-Row-Access-Audit.md) and [item conversion gap](MM6-F4-Shop-Item-Record-Conversion-Gate.md) are checked, then connect the title-local owner to service writers and test live/disk late-failure atomicity. Bans and bounty need their own original row/lifetime evidence.

The ignored original-stock checker passed accepted executable identity, row/stride/boundary, raw save span and cross-map autosave assertions. Linked `MM6ShopGuildClockFields.*` passed 2/2. No proprietary data entered the repository, and the normal MM6 SaveLoad guard remains closed.

See also: [Shop Stock Tuple and Autosave](MM6-F4-Shop-Stock-Tuple-and-Autosave.md), [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Shop Guild Clock Save](MM6-F4-Shop-Guild-Clock-Save.md), [Shared-Edge Refresh](MM6-F4-Shop-Shared-Edge-Refresh.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
