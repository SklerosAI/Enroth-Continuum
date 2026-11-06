# MM6 F4 shop item conversion gate

Status, 2026-11-06: the original MM6 shop stock owner must keep **all 28 bytes per item**. The current 36-byte `Item_MM7` snapshot is not a reversible MM6 shop item representation. A detached raw `SHOP` v1 exists, but no conversion into a live `Item`, house service or required save section is authorized.

The accepted F3-matched executable clears complete 28-byte stock records, writes the first six 32-bit words through generation, transfers all seven words on a successful purchase, and clears only bytes 0–24 of the source. [MMExtension's pinned MM6 item structure](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/01%20common%20structs.lua#L503-L529) gives the following field labels. The [pinned editor's embedded item layout](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/Item.java#L1184-L1202) agrees on widths and offsets for an item inside a ground object. These are useful independent schemas; neither proves every consumer in this original executable.

| MM6 bytes | Schema label and original evidence | Live conversion gap |
| --- | --- | --- |
| 0–3 | Item number; original generation, display and purchase paths use it. | Validate the complete MM6 item table and category before creating a live `Item`. |
| 4–7 | Bonus; original generic item generator writes the word. | MM7 interprets it as standard enchantment or potion power by MM7 category. That MM6 interpretation is not established for all items. |
| 8–11 | Bonus strength; original generator writes the word. | Complete MM6 bonus behavior and reversible `Item` mapping remain unchecked. |
| 12–15 | Second bonus or gold value; original generator writes the word. | MM7's special enchantment/gold union is category dependent. |
| 16–19 | Charges; original generator writes the word. | Category-specific charge behavior and full round-trip remain unchecked. |
| 20–23 | Condition/flags; original shop/guild generation sets the identified bit. | Identified and broken are labeled; MM7 flag semantics must not be assumed for every other bit. |
| 24 | Body location; purchase clears this byte. | The fork owns equipped slots outside `Item`, so stock and equipped-slot meaning need an MM6-specific bridge. |
| 25 | Maximum charges; purchase retains this byte. | Nonzero live behavior and conversion are unverified. |
| 26 | Owner, marked uncertain by MMExtension; purchase retains this byte. | MM7 treats the same position as a lich-jar character index. A direct copy would assign a different meaning. |
| 27 | Skipped/padding byte; purchase retains it. | Preserve exactly; no normalization or inferred meaning. |
| 28–35 | No MM6 bytes. | MM7 has an eight-byte enchantment expiration field here; it cannot be recovered from an MM6 record. |

The fork's [MM6 chest projection](../src/Engine/MM6/MM6ChestRuntimeProjection.h) attempts only a guarded first-24-byte `Item_MM7` round trip and keeps the remaining four MM6 bytes in a raw sidecar. That limited chest check does not establish shop generation or purchase conversion. In particular, `Item_MM7::reconstruct` uses MM7 item categories for the two bonus unions and interprets byte 26 differently.

**Safe owner policy:** retain the existing physical 1,440-record `ShopStockFields` array as the sole authority, including the shop/guild alias and sold-slot trailing bytes. `SHOP` v1 may round-trip the exact raw records in isolation. No inherited MM7 commerce array, live `Item` projection or normal MM6 SaveLoad admission should be added until a title-local MM6 item mapping proves every nonzero field it changes and preserves unknown bytes. A nonzero native saved shop record, generated/purchased record checks and late-failure tests are still needed.

See [shared-row access audit](MM6_F4_SHOP_SHARED_ROW_ACCESS_AUDIT.md), [shop owner field gate](MM6_F4_SHOP_OWNER_FIELD_GATE.md), and [purchase write](MM6_F4_SHOP_PURCHASE_WRITE_GATE.md). Exact original-executable offsets and private observations remain ignored locally.
