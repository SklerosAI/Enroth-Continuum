---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-Save-Remainder]]"]
tags: [mm6, f4, save, transaction]
---

# MM6 F4 Save Transaction Checklist

The sixteen-section `.ec6` assembly is an intentionally partial candidate. Its CRC, section set, field bounds, read-only slot inventory and disk replacement do not yet provide a playable save/load path. `SaveLoad.cpp` rejects MM6 before the inherited MM7 serializer or live mutation.

The [metadata packet](../project/MM6_EC6_METADATA_F4.md) records the transaction ordering and exact remaining runtime owners. Current bounded pieces include detached `Party` field-family adapters, real `NPCStats` and 67-map-buffer capture, the bounded active-map changed-state writer, and a joint simulation `Timer`/50-overlay capture and copy-stage seam. The latter validates loaded overlay-table bounds and returns a paused clock with discarded process-frame timing. It does not rebind overlay PIDs to a reconstructed world.

| Owner to complete before live entry | Unrepresented or unproved state |
| --- | --- |
| `Party` and `PartyTimeStruct` | MM6-keyed shop/guild/bounty clocks and stock; fine, death, prison and bounty statistics; alignment, persistent movement/flight/fall flags; generated-artifact identity and flags. Existing house/item enum ranges are inherited MM7 values and require MM6 identity proof. |
| Four `Character` objects | Quick spell/page; Lloyd beacons, map identity and images; previous face/voice; eight private attack/health/mana bonus bytes. `CHAR` v2 covers basic attributes plus stat modifiers, birth year and pure-potion gates only. |
| Active indoor/outdoor world | Validate all 67 deltas against private bases, instantiate/stage the active map and its actors/items/chests, then verify PID and item-template references before any global swap. Outdoor live filename identity remains open. |
| Runtime UI and actions | Finish active rest/turn rejection or restore policy, cursor image after `HELD`, pending spell/action policy, and UI reset after a successful commit. |

The load transaction must select a semantically valid primary or backup without disk mutation, decode the **complete** mandatory manifest, validate loaded MM6 resources and all map deltas, stage every owner on copies, then commit only after no stage can throw. Only after that should it restore a recoverable backup or expose the MM6 menu route. The current atomic file writer protects a disk slot, not the simultaneous assignment of `Party`, timer, overlays, NPCs, map buffers and active world. F5 still requires save, quit, relaunch, load and rendered-state checks. No original installation or partial MM6 process is needed for this F4 checklist.

### `CHAR` v3 bonus-byte gate

The [core field packet](../project/MM6_EC6_CORE_FIELDS_F4.md) names all eight current `Character` `char` members and their current readers/reset paths. The first four affect attack/damage and two affect maximum HP/SP; the two `*_related` bytes have no active non-reset gameplay reader. All eight are copied by the inherited MM7 snapshot, which is not MM6 identity evidence. The accepted F3 per-family `.ec6` design and PE creation report do not assign native MM6 member identity, signedness or lifetime; no native whole-save was read. Do not upgrade `CHAR` by copying MM7 fields. Resolve original member/consumer mapping or explicitly accept a raw current-runtime preservation contract with a reset/load policy, then add v3 capture/stage and nonzero tests. Live SaveLoad remains guarded.
