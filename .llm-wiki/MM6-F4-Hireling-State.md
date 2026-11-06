---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-NPC-State]]", "[[MM6-F4-Known-Save-Assembly]]", "[[Source-MM6-NPC-Profession-F2]]"]
tags: [mm6, f4, save, npc]
---

# MM6 F4 Hireling State

The [HIRE packet](../project/MM6_EC6_HIRELING_FIELDS_F4.md) defines a bounded `.ec6` section for the two party hireling copies, their display names and sacrifice progress. It is an offline runtime snapshot with title-guarded capture and party-copy staging, not a connected save path or native MM6 wire format.

`HIRE` reuses the full `RESC` NPC record codec, retaining ability-use, flags, profession, dialogue IDs and names. Sacrifice flag and durations are stored separately; the inherited MM7 save aliases them onto NPC event fields, which would lose dialogue state here. The section validates counts, names, record bounds and nonnegative sacrifice time before assigning a decoded result.

The derived nonhire follower count and hireling scroll position are not part of this section. The [identity reconciliation packet](../project/MM6_EC6_HIRE_RECONCILIATION_F4.md) shows that no strict match can safely be required: table-only quest followers, generated party copies, event-cleared table flags, name rewrites and duplicate retained source identities all defeat a name/portrait/profession key. A bounded diagnostic reports candidate counts without mutating or rejecting valid state.

MM6's runtime follower roster now uses a checked 16-bit row index for the 501-slot NPC table, with party hirelings first and table-only followers in row order. The retained F3-matched NPC table has an active upper bound of 399, beyond the inherited one-byte roster limit. `collectFollowerIndices` rejects invalid active counts and unnamed hired rows; the MM7/MM8 roster representation remains unchanged. This fixes addressability only: duplicate-name exclusion is still inherited, and roster positions are not stable hireling source IDs. MM6 profession effects, stable source-ID provenance, initialized-party adapter testing, SaveLoad integration and F5 hire/dismiss/sacrifice persistence remain open.

MM6 NPC lookup also checks the fixed 501-row table, 100-slot generated table and negative flattened-roster IDs before indexing. It rejects invalid IDs, including the minimum signed integer, instead of forming an out-of-bounds pointer. `getNPCType` uses the same resolver. Table slots 399–500 remain addressable but are not thereby proven populated. Duplicate-name dismissal still may clear multiple table flags, and a party copy cannot establish which source row was hired. See the [identity and lookup packet](../project/MM6_EC6_HIRE_RECONCILIATION_F4.md) for the exact bounds and remaining F5 checks.

See also: [NPC State](MM6-F4-NPC-State.md), [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [F4 Offline Seams](MM6-F4-Offline-Seams.md).
