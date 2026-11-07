---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]", "[[MM6-F4-Bounded-Party-Inventory]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, item, save, party]
---

# MM6 F4 Raw Party Inventory Stage

The [implementation packet](../project/MM6_F4_RAW_PARTY_INVENTORY_STAGE.md) defines a detached `INVR` to `Party` inventory candidate that retains the native 28-byte item records.

`stageRawPartyInventories` requires MM6 title selection and the active title-local item table. It uses the existing four-character bounded projection, then installs each 138-slot `CharacterInventory` on a copy of the supplied `Party`. The source `RawInventoryFields` remains beside that copy. `captureRawPartyInventories` replays the original raw records and index tables, overlays the four edited inventories and requires a reversible capture before returning a new raw owner. The original Party and raw input are never assigned.

The current adapters accept source-typed, common-rarity plain records across all 21 MM6 table item types when bonus unions are zero, including a scroll ID that overlaps the inherited MM7 potion range. They also accept common equipment with a mapped positive standard bonus, and a single [Antique value-only special case](../project/MM6_F4_ANTIQUE_ITEM_PROJECTION.md) with native second bonus ID 58. These are synthetic category checks, not proof that every shipped nonzero record is representable. A present native record carries all 28 bytes through `Item::mm6RawRecord`; capture preserves owner/padding tails and unreferenced physical records while allowing only the adapter's proved edits.

Nonzero potion-power unions, special enchantments other than Antique, artifact/relic effects, malformed physical placement, absent or substituted carriers and unsupported item edits reject before any live mutation. The pinned [MMExtension MM6 item layout](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/01%20common%20structs.lua#L503-L529) confirms the word and tail positions; it does not establish every gameplay interpretation. The local original-item picker proves standard and special writes for generated equipment, but potion placement, general special effects, original Antique pricing and full artifact effects remain open. The function stages inventories only; it does not restore saved Party fields, cursor, shops, map, UI or beacons, and it does not open normal MM6 loading.

A focused arm64 object build and linked synthetic harness passed **3/3** original tests. They cover all 21 source table types across four characters, standard armor edit, native tail preservation, byte-exact capture, late unsupported unions, foreign title/table, carrier loss and item deletion. The Antique follow-up passed **3/3** separate linked synthetic tests for value, raw byte preservation, reversible edit and late failure. No original game files or private item rows were committed.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Bounded Party Inventory](MM6-F4-Bounded-Party-Inventory.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
