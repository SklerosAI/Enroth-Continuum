---
type: synthesis
up: "[[MM6-F4-Generated-NPC-Identity-Save]]"
related: ["[[MM6-F4-Gameplay-RNG-Save]]", "[[MM6-F4-Spawned-Actor-Owner-Decision]]"]
tags: [mm6, f4, npc, rng, save]
---

# MM6 F4 Pre Event GNPC Replay

The [source-backed packet](../project/MM6_F4_PRE_EVENT_GNPC_REPLAY.md)
traces the narrow generated-NPC and gameplay RNG handoff before map events.

After actor rebuild, `_461103_load_level_sub()` generates peasant NPC rows
and draws `grng`; `onMapLoad()` follows and can consume random state. A future
replay must reconcile spawned actors, bypass only the generated-row loop,
publish saved GNPC/RNGS and then run map events. Chest, arena, decoration and
cursor work in the same subroutine must still run.

The detached read-only agreement helper checks title, map, RNG type, fresh
row cursor, actor slots, monster and peasant anchors, contiguous saved rows
and static links. Its linked suite passed 2/2. It does not publish a live
owner, admit EC6 v2 or prove fresh-child continuation.

See also: [Generated NPC Identity Save](MM6-F4-Generated-NPC-Identity-Save.md), [Gameplay RNG Save](MM6-F4-Gameplay-RNG-Save.md), [Spawned Actor Owner Decision](MM6-F4-Spawned-Actor-Owner-Decision.md).
