# MM6 shop purchase-write and record gate — F4

Status, 2026-11-06: the accepted F3-matched original executable proves a bounded **physical stock-record address**, but this follow-up still does not prove a complete purchased-record update. No detached `SHOP` record codec or live owner is added; normal MM6 SaveLoad and house services remain guarded.

## What the original trace establishes

- The two shop menu branches select distinct banks. The selected slot and display scan use the current original MM6 house row and a 12-position row of 28-byte records. The first 32-bit word is an item ID; zero suppresses a displayed slot.
- The spell-guild generator and display path use the same 12-position, 28-byte shape. The final active shop row's second bank and the first active guild row address the same twelve physical records. The existing [physical index helper](../src/Engine/MM6/MM6ShopStockIdentity.h) expresses this alias and rejects foreign titles and out-of-range keys.
- The selected shop-record pointer is passed into an item display call. The inspected direct bank references and menu dispatch do not establish the later purchase operation that mutates the slot. In particular, zero-ID display suppression does **not** prove that a purchase zeroes only the ID, clears all 28 bytes, or transfers/rewrites another field.
- The original raw Party save/read span includes the stock region. That is persistence of the original process memory, not a complete contract for this fork's live keyed owner.

## Independent editor cross-check and mismatch

The pinned [MM6 `PartyBin.java` research-editor implementation](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/PartyBin.java) reads two 47-row shop lists followed by a separate 22-row guild list of `ShopContents`. Relative to the original executable's first-shop-bank origin, those consecutive lists start at physical row indices 1, 48 and 95. The original guild address formula starts its first active row at physical index 94, which is also the last shop row of bank two. Thus the editor's guild list starts **one 12-slot row later** than the original first active guild address. The editor itself treats its counts as uncertain. Its [28-byte item interpretation](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/ContainedItem.java) is useful for hypotheses, but cannot override the original address or supply purchase semantics. The arithmetic mismatch is an inference from the two sources, not a claim that a native save has been observed with nonzero shared slots.

## Codec decision and exact next proof

A pure 28-byte copy could round-trip an arbitrary detached byte array, but it would not validate the original purchase outcome, determine which shared record a service owns, or capture changed live stock from this fork. A codec restricted to the item-ID word would discard the other 24 bytes. A pair of disjoint 47-row shop arrays plus 22-row guild array would assign two logical authorities at the shared edge and shift the guild layout. None is a safe `SHOP` owner contract. `SCLK` remains the sole represented commerce field; it owns the separately proved refresh deadlines.

The smallest prerequisite is to trace the selected-slot purchase handler's write and next read for both shop banks and the guild path, including all 28 bytes and the shared edge. Compare that result with a nonzero owner-local native `party.bin` record. Then specify one MM6-keyed physical owner and a deterministic alias policy, connect the current MM6 generation/purchase paths to it, and test a nonzero sale/refresh round-trip plus duplicate-alias and late-stage-failure rejection on detached copies. Keep ban and bounty clocks/targets separate until their own row and lifetime evidence exists.

Verification: the ignored F3 identity/stock/refresh/raw-save static checker passed. This was a read-only source audit; no native save, game process, C++ implementation or proprietary bytes were added to Git.

See [shop owner field gate](MM6_F4_SHOP_OWNER_FIELD_GATE.md), [stock tuple and autosave](MM6_F4_SHOP_STOCK_TUPLE_AND_AUTOSAVE.md), and [save transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md).
