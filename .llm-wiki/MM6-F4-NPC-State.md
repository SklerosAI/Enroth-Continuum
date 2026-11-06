---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[Source-MM6-NPC-Profession-F2]]", "[[Source-MM6-Save-Field-Register-F2]]"]
tags: [mm6, f4, npc, save]
---

# MM6 F4 NPC State

The [NPC field packet](../project/MM6_EC6_NPC_FIELDS_F4.md) describes a bounded, title-specific `.ec6` `RESC` runtime snapshot and copy-staging adapter. It is an offline field family, not a connected save transaction or native MM6 save format.

The schema stores all 501 mutable NPC slots, 51 group words and the runtime active count. It preserves every current `NPCData` scalar, including numerically opaque flag bits, generic MM6 profession IDs and unproven event/news fields. An aggregate check of the ignored F3-matched `npcdata.txt` copy found 398 numeric source records (IDs 1–398), yielding active bound 399; the F2 profession ledger's 70 records are the generic-profession subset. Decode requires exact version/counts and validates bounds before returning a staged `NPCStats` copy; original tables and unrelated runtime members remain on that copy.

The retained MM6 icons copy has no `npcgroup.txt`, and the current MM6 table loader does not import one; group defaults and event mutation still need reconciliation. `pAdditionalNPC`, transient counters, hireling copies, profession behavior and NPC sacrifice clocks are outside this payload. SaveLoad integration, atomic writes, save/quit/relaunch and graphical NPC checks remain open.

See also: [F4 Offline Seams](MM6-F4-Offline-Seams.md), [F2 NPC Profession](Source-MM6-NPC-Profession-F2.md), [F2 Save Fields](Source-MM6-Save-Field-Register-F2.md).
