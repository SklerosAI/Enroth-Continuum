---
type: synthesis
up: "[[MM6-F4-EC6-V3-Manifest]]"
related: ["[[MM6-F4-EC6-V3-Full-Session-Preflight]]", "[[MM6-F4-Pre-Event-Checkpoint]]", "[[MM6-F4-EC6-V3-Outdoor-Actor-Map]]"]
tags: [mm6, f4, save, reload, headless, ec6]
---

# MM6 F4 V3 Post Entry Map and Pre Event

The [source-backed packet](../project/MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md)
records two bounded v3 seams: post-entry outdoor DDM capture and a test-only
ACTR/GNPC/RNGS replay context before map events. Neither is a production
save or load route.

The map capture keeps original native actor/chest records and opaque bytes,
patches supported reset/visibility/sprite headers, and refuses unsupported
ground item, chest, actor-identity or sprite changes. One fractional sprite
height is accepted only when reconstructing from its exact DDM integer
position and applying the loader's floor step reproduces the live position.
The private-backed first-map retained-DDM/live-owner probe passed **1/1**.
It does not prove later maps, item mutations or a save–quit–reload.

A separate no-write first-map full-session v3 candidate currently refuses
at public class `map-preflight` (**0/1**). Its exact cause remains under audit; it
also guards unrepresented commerce and cursor state. The passing narrow map
probe does not admit the broader session or open an atomic write.

The test-only pre-event scope validates the staged v3 map/ACTR/GNPC/RNG
agreement and unchanged chest/active-DDM owners, publishes actors and
generated NPC rows, bypasses only generated-row creation in
`_461103_load_level_sub()`, and restores RNG after that subroutine's other
work but before `onMapLoad`. Linked pre-event/checkpoint checks passed
**7/7**. Disposable v2 writer and fresh-child abort controls passed
**1/1** each after integration; MM7 headless control passed **1/1**.
No v3 child replay, disk write, post-world admission, normal SaveLoad or
Atlas Play is enabled. ACTR still rejects actor items and nondefault buffs.
A disabled strict-input-only fresh-child v3 GameTest is present but unrun;
it requires a separately admitted disposable v3 slot.

See also: [EC6 V3 Full Session Preflight](MM6-F4-EC6-V3-Full-Session-Preflight.md),
[Pre Event Checkpoint](MM6-F4-Pre-Event-Checkpoint.md),
[EC6 V3 Outdoor Actor Map](MM6-F4-EC6-V3-Outdoor-Actor-Map.md).
