---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-Save-Remainder]]"]
tags: [mm6, f4, save, transaction]
---

# MM6 F4 Save Transaction Checklist

The sixteen-section `.ec6` assembly is an intentionally partial candidate. Its CRC, section set, field bounds, read-only slot inventory and disk replacement do not yet provide a playable save/load path. `SaveLoad.cpp` rejects MM6 before the inherited MM7 serializer or live mutation.

The [metadata packet](../project/MM6_EC6_METADATA_F4.md) records the transaction ordering and exact remaining runtime owners. Current bounded pieces include detached `Party` field-family adapters, real `NPCStats` and 67-map-buffer capture, the bounded active-map changed-state writer, and a joint simulation `Timer`/50-overlay capture and copy-stage seam. The [partial coordinator](MM6-F4-Partial-Stage-Coordinator.md) now validates all 67 base/delta pairs before staging NPC/map and timer/overlay copies in one result. It does not rebind overlay PIDs to a reconstructed world or stage Party.

| Owner to complete before live entry | Unrepresented or unproved state |
| --- | --- |
| `Party` and `PartyTimeStruct` | MM6-keyed shop/guild/bounty clocks and stock; fine, death, prison and bounty statistics; alignment, persistent movement/flight/fall flags; generated-artifact identity and flags. The [shop save identity audit](../project/MM6_F4_SHOP_SAVE_IDENTITY.md) shows current stock/clock/bounty arrays use inherited MM7 `HouseId` intervals, while accepted F3 MM6 evidence has no matching state key. |
| Four `Character` objects | Lloyd beacons, map identity and images; previous face/voice; eight private attack/health/mana bonus bytes. `CHAR` v3 also copy-stages quick spell and last spellbook page as current-runtime preferences. |
| Active indoor/outdoor world | Validate all 67 deltas against private bases, instantiate/stage the active map and its actors/items/chests, then verify PID and item-template references before any global swap. Outdoor live filename identity remains open. |
| Runtime UI and actions | Finish active rest/turn rejection or restore policy, cursor image after `HELD`, pending spell/action policy, and UI reset after a successful commit. |

The load transaction must select a semantically valid primary or backup without disk mutation, decode the **complete** mandatory manifest, validate loaded MM6 resources and all map deltas, stage every owner on copies, then commit only after no stage can throw. Only after that should it restore a recoverable backup or expose the MM6 menu route. The current atomic file writer protects a disk slot, not the simultaneous assignment of `Party`, timer, overlays, NPCs, map buffers and active world. F5 still requires save, quit, relaunch, load and rendered-state checks. No original installation or partial MM6 process is needed for this F4 checklist.

The `SHOP`/bounty owner is blocked before codec work: the [557-row save-key matrix](MM6-F4-Shop-Save-Key-Matrix.md) identifies candidate house rows but no proved MM6 stock, clock or bounty tuple key. Classify every active MM6 shop, spell guild and council/town hall by stable title-specific behavior; give the MM6 party owner matching storage; prove stock cardinality, refresh/ban timing, bounty tuple and item/monster identity; then stage the complete keyed state on copies with nonzero late-failure tests. Serializing the inherited MM7 array ranges would silently lose MM6 spell guilds and candidate bounty houses. The current MM6 house view is presentation-only, so inherited service consumers are not active MM6 behavior proof.

### Later `CHAR` bonus-byte gate

The [core field packet](../project/MM6_EC6_CORE_FIELDS_F4.md) names all eight current `Character` `char` members and their current readers/reset paths. The first four affect attack/damage and two affect maximum HP/SP; the two `*_related` bytes have no active non-reset gameplay reader. All eight are copied by the inherited MM7 snapshot, which is not MM6 identity evidence. The accepted F3 per-family `.ec6` design and PE creation report do not assign native MM6 member identity, signedness or lifetime; no native whole-save was read. Do not add these bytes by copying MM7 fields. Resolve original member/consumer mapping or explicitly accept a raw current-runtime preservation contract with a reset/load policy, then add a later CHAR capture/stage version and nonzero tests. Live SaveLoad remains guarded.
