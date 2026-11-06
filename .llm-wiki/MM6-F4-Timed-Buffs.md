---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[Source-MM6-Save-Field-Register-F2]]"]
tags: [mm6, f4, save, buffs]
---

# MM6 F4 Timed Buffs

The [buff field packet](../project/MM6_EC6_BUFF_FIELDS_F4.md) documents a title-specific `.ec6` `BUFF` field family for current runtime party and character effects. It is an offline codec and copy-staging adapter, not a native MM6 save format or a connected live save.

It records 20 party and 4×24 character slots, including expiration, power, mastery, overlay ID, caster and the `isGM` flag. The current NPC Wind/Water Master path sets `isGM` on a Master-level buff, so the flag must survive even though MM6 characters have no Grandmaster skill. Malformed counts, ticks, mastery, caster and overlay indexes reject before output mutation. Staging changes a party copy without calling overlay-mutating buff methods.

The existing `EFFT` section owns condition clocks; the F3 logical effect grouping did not specify original buff wire widths. The integrated partial known-save assembly now requires `BUFF`. Overlay reference consistency with `VISU`, exact MM6 effect semantics, initialized-party runtime testing, SaveLoad integration and F5 save/relaunch remain open.

See also: [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [F2 Save Fields](Source-MM6-Save-Field-Register-F2.md), [F4 Offline Seams](MM6-F4-Offline-Seams.md).
