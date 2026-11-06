# MM6 chest record transaction — detached F4 join

Status: **staged and native-tested, not wired to the live MM6 load or save path** (2026-11-06). The guarded `mm6::generateChestItemsForTitle` still rejects MM6 before chest, RNG or artifact mutation. MM7 continues through its inherited path.

## Input and atomic result

`mm6::stageChestRecordGeneration` takes all twenty 4,204-byte chest records, zero-based map treasure level, RNG state, MM6 `ItemTable`, party-owned 400–429 artifact flags and already-generated flags. It validates the title, eight chest-type IDs, all 2,800 item IDs, 140 signed grid cells per chest, and blank non-ID bytes in any zero/negative generated target. It also rejects a chest with a marker and a nonzero placement grid. A failed validation or nested picker returns no result and changes no caller state. The output is a copied record set, generated flags, next RNG state and counts. Fixed positive item records retain all 28 bytes, including opaque tail bytes.

The outer plan now exposes source-order selections, so the transaction can join each empty/gold/ANY/direct decision to its exact slot without repeating RNG draws. Gold writes its MM6 item ID and amount at item offset +12. ANY uses the retained-table caller and writes the known fields at item offsets +0, +4, +8, +12, +16 and +20, leaving the final four bytes zero. Direct artifacts write their bounded ID with zero other fields. All writes are to a local copy. The original generator's marker-free entry draw remains observable in the returned RNG state.

The **personal-fork policy** for direct tier-seven artifacts is deliberately safe and divergent from the original executable's out-of-bounds slot-index mark: only IDs **400–429**, excluding party-owned or already-generated IDs, with a combined cap of thirteen. A no-candidate result clears the marker. ID 430 is not considered safe. The tier-six ANY artifact trial keeps its separately proved 400–429/generated-cap behavior. This policy must remain explicit in any eventual runtime integration and save semantics.

In a read-only aggregate check of the retained 67-map baselines, all 186,377 empty item slots, 943 marker slots and 280 fixed positive slots had zero bytes after the four-byte ID. That supports a blank generated-target precondition for those baselines, but does not prove changed-save behavior. No private assets, exact item profiles or original executable bytes are stored here.

## First-open placement is a second transaction

`mm6::stageFirstChestRecordPlacement` accepts one generated record, the MM6 chest-type bytes and an icon-size callback. It validates the detached type layout and all item IDs, runs the proved 9×9 first-open placement over a 140-cell in-memory grid, and stages the flag, wand-charge fallback and all **140 signed little-endian grid cells** into a copied 4,204-byte record. It leaves other item fields and opaque bytes intact. Placement belongs to first chest open; map-level generation must not perform it early. A failed icon lookup, malformed record, unsupported title or failed fit model publishes nothing.

## Verification and live gate

The focused arm64 native test link exercises marker-free RNG and fixed-byte preservation, gold, ANY item fields, direct-artifact bound/cap, all six active marker levels across seven map levels, 9×9 placement and wand charge, plus malformed/foreign-title atomic rejection. This is an offline transaction test, not a game launch or graphical chest observation.

The live guard remains closed because `MM6IndoorSnapshot` and `MM6OutdoorDeltaSnapshot` still reject a changed chest grid during runtime projection; outdoor projection also copies only the first 26 bytes of an item, whereas indoor copies 28. A later integration must reconcile MM6 `ChestInventory`/`Item` ownership, the live RNG and generated-artifact state, first-open UI/trap/pickup behavior, changed-grid save and `.ec6` save–quit–load. It should then run headless and owner-visible graphical acceptance without using the protected MM7 or reserved MM8 runtime as a scratch area. See the [generation audit](MM6_F4_CHEST_GENERATION_AUDIT.md), [outer plan](MM6_F4_CHEST_OUTER_PLAN.md), [grid wire](MM6_F4_CHEST_GRID_WIRE.md) and [interaction gate](MM6_F4_CHEST_INTERACTION_GATE.md).
