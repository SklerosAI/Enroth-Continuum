# MM6 held-item save field — F4

Status: bounded current-runtime `.ec6` field and copy-staging, 2026-11-06. This is not native `.mm6` save compatibility or a live SaveLoad hook.

## Source and lifetime audit

`Party::pPickedItem` is an `Item`, separate from the four character inventories. `Party::setHoldingItem` first calls `placeHeldItemInInventoryOrDrop()` for any existing item, then assigns the new item and changes the global mouse bitmap and click offset. `Party::takeHoldingItem` clears the item and cursor. `dropHeldItem` materializes a sprite containing the item; `Character.cpp`, `Chest.cpp`, shops, NPC topics and popup potion/enchantment actions also read or transfer `pPickedItem`. Saving only character `ITEM` inventory records would therefore lose an item held during a save.

The field persists **item state**, not pointer/cursor state. The current `Item` scalars are ID, potion power, gold amount, optional standard enchantment and strength, special enchantment, wand charges and maximum, flags, lich-jar character index and temporary enchantment expiry. The `HELD` field stores every scalar, preserving unknown flag bits. The MM7-only lich-jar index must be `-1` for this bounded MM6 field; a nondefault value fails closed. An absent item has one canonical four-byte form, regardless of stale scalar values left in the runtime object when `itemId` is null.

The ignored F3-matched retained item-footprint check summarized in [inventory restore](MM6_F4_INVENTORY_RESTORE.md) found 580 numbered item rows, 578 usable icons and two placeholders. This lane did not reopen the original installation or copy any item names/rows. It uses the same current MM6 item-ID ceiling and loaded-template check as inventory staging. No native whole-save field offset is claimed.

## `HELD` v1 contract

`MM6HeldItemSaveFields.*` adds a required tag `HELD`, distinct from inventory `ITEM`. The payload starts with version 1, presence byte 0/1 and two zero reserved bytes. Empty state is exactly four bytes; present state appends ten 32-bit values and one 64-bit expiry tick, all little-endian (52 bytes total). The codec rejects a missing/optional/duplicate section, bad shape/version, noncanonical empty state, item ID outside 1–580, enchantment indices beyond current supported MM6 ranges, negative charges/expiry and a nondefault lich-jar index. The container decoder checks title/version/CRC before `loadHeldItemBeforeMutation` assigns its destination.

`captureHeldItem` and `stageHeldItemParty` require an active MM6 engine title. For a present item they require the loaded MM6 item table to have a nonempty icon name and a usable 1–14 by 1–9 inventory footprint. Staging reconstructs an `Item` on a **copy** of `Party` and changes only `pPickedItem`. It does not call `setHoldingItem`, `takeHoldingItem`, inventory placement, sprite drop, or the global mouse. No partial item or cursor state reaches the live session on rejection.

After a full save transaction commits, the UI adapter must display the restored item's icon and choose a safe cursor offset (the original click offset is transient and may be recentered). The current `setHoldingItem` cannot be used directly for that step because it would move/drop any old live held item before the transaction is complete. Test held wand/potion/quest item on screen after save/quit/relaunch/load, and confirm cursor image, potion/charge values and one-item-only ownership. The whole-save required manifest must include `HELD` before SaveLoad can use it.

## Offline verification and remaining gates

Two focused C++ tests pass for empty/present exact round-trip, signed and opaque flag values, bad ID/lich index, malformed valid-CRC shape/length, optional-section and wrong-title rejection, and no destination mutation. The runtime adapter passes C++23 syntax and style checks. In-game item interactions and cursor restoration remain F5 work; no partial MM6 process was launched.
