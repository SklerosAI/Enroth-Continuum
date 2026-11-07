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

A separate no-write first-map full-session v3 candidate passed detached
`stageV3FullSessionPreflight()`. The event classifier then confirmed exact
native event bytes and zero generic `mapVars`, but found inherited generic
`decorVars` nonzero after map load: `event-decor-bound-high` (**0/1**).
That value is bound to an interactive decoration's assigned high
`eventVarId`, not an unbound transient. This supersedes `event-legacy-decor`,
`map-preflight` and broad `persistent-event-join` classes. Initial respawn
calls `RespawnGlobalDecorations()` before
`PrepareDecorations()`, filling generic `decorVars[0..123]`; a stamped
no-respawn DDM reload skips that call. Some event draws consume RNG, and
no exact mapping to MM6's native 200 event bytes is proved. Zeroing the
generic values would lose live state. The later detached
[v4 EVRT owner](MM6-F4-EC6-V4-Event-Runtime.md) retains those arrays
independently; it has not enabled a durable v4 save.
The candidate also guards unrepresented commerce and cursor state. Neither
passing stage opens an atomic write.

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
