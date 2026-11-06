---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, save, runtime, fields]
---

# MM6 F4 Save Remainder

This page is the handoff for durable runtime state still outside the `.ec6` core, durable, metadata, NPC and map field families.

The exhaustive [field, consumer and lifetime matrix](../project/MM6_SAVE_REMAINDER_F4.md) was checked against current `Party`, `Character`, `NPCData`, inherited MM7 snapshots and the ignored F3 report scope. The F3 content signoff does not contain native whole-save offsets; current runtime preservation and MM6 game-rule truth must remain separate claims.

| State group | What must survive | Current gate |
| --- | --- | --- |
| Resistance pairs | Eleven signed base and eleven signed bonus fields per character | Required `RSTS` v1 now round-trips and copy-stages all 88 current values under an MM6 title guard. Channel effects in MM6 combat remain unproved. |
| Character and party buffs | 24 character and 20 party `SpellBuff` slots, including expiry, power, mastery, overlay, caster and GM flag | No codec. Reconcile overlay references with `VISU` and MM6 slot meanings before stage. |
| Recovery and clocks | Character recovery, turn-based recovery, regeneration, rest/day/calendar, shop/guild/bounty refresh and bans, 10 counters, 29 history times and 20 miscellaneous event times | No codec. Time validity and title-specific house/event indices must be checked. |
| Held and shop items | Cursor-held item and standard/special/guild stock, with all item scalars, stock refresh and bans | Required [HELD](MM6-F4-Held-Item.md) now captures and copy-stages the held item; shop/guild stock is still open. Cursor image/offset is transient. |
| Hirelings | Two copied NPC records, names, ability-use flags, follower count and sacrifice durations | Global `RESC` NPC/group section does not restore party copies. Resolve identity and profession joins. |
| Generated artifacts | Current 29 booleans and item generation limit | Inherited 500–528 band requires MM6 meaning proof before a title-specific codec. |
| Other campaign state | Stat modifiers, pure potion gates, daily spell counters, fines/bounties, flight/fall, beacons and images, quick spell/page | Not yet encoded. See the project matrix for individual consumers, lifetimes and restoration policy. |
| Frame/UI state | Animation frames, walking sound, floor face, delayed reactions, rounding remainder, action queue and cursor image | Reset or rederive after load, subject to pending-action check. |

`RSTS` is a versioned, exact-size, required section. Full-container title/CRC validation and field decoding finish before the destination is assigned; runtime staging changes only a party copy. The full arm64 unit target linked and two focused Google Test cases passed signed round-trip and malformed/wrong-title no-mutation checks. The runtime adapter compiled, but there was no MM6 process launch. The partial known-save manifest now requires `RSTS`; the complete transaction still needs the remaining state.

Next agents should read the full [project matrix](../project/MM6_SAVE_REMAINDER_F4.md) before selecting a field family, check for new ownership of the whole-save assembler, and keep SaveLoad disabled until mandatory sections and atomic recovery are integrated. The retained F3 reports remain ignored and private.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Save Field Register](Source-MM6-Save-Field-Register-F2.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
