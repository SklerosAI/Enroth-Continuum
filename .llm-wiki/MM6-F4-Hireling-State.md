---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-NPC-State]]", "[[MM6-F4-Known-Save-Assembly]]", "[[Source-MM6-NPC-Profession-F2]]"]
tags: [mm6, f4, save, npc]
---

# MM6 F4 Hireling State

The [HIRE packet](../project/MM6_EC6_HIRELING_FIELDS_F4.md) defines a bounded `.ec6` section for the two party hireling copies, their display names and sacrifice progress. It is an offline runtime snapshot with title-guarded capture and party-copy staging, not a connected save path or native MM6 wire format.

`HIRE` reuses the full `RESC` NPC record codec, retaining ability-use, flags, profession, dialogue IDs and names. Sacrifice flag and durations are stored separately; the inherited MM7 save aliases them onto NPC event fields, which would lose dialogue state here. The section validates counts, names, record bounds and nonnegative sacrifice time before assigning a decoded result.

The derived nonhire follower count and hireling scroll position are not part of this section. The later whole-save transaction must reconcile HIRE identities with RESC hired flags and decide how to restore derived/UI state. MM6 profession effects, initialized-party adapter testing, SaveLoad integration and F5 hire/dismiss/sacrifice persistence remain open.

See also: [NPC State](MM6-F4-NPC-State.md), [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [F4 Offline Seams](MM6-F4-Offline-Seams.md).
