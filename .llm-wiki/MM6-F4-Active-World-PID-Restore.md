---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Partial-Stage-Coordinator]]", "[[MM6-F4-World-Save-Runtime]]"]
tags: [mm6, f4, save, world, pid]
---

# MM6 F4 Active World PID Restore

The [source packet](../project/MM6_F4_ACTIVE_WORLD_PID_RESTORE.md) records the active saved-map actor bound, the inherited world-load order, and the remaining restore transaction gate.

The detached `stageKnownWorldAndVisuals()` path validates all 67 MM6 base/delta pairs, obtains the active delta actor count, and rejects an active overlay whose packed PID is not an actor PID or points beyond that saved count. This is a saved-record reference check. It cannot establish identity after map reconstruction.

`DoPrepareWorld` resets live level resources before reconstructing actors, sprites and chests. Indoor/outdoor loaders may substitute a fresh delta for initial or timed respawn. Later initialization/spawn, debug actor removal, NPC-ID rewriting, gated chest generation, and `onMapLoad` can change the world. The inherited loader also changes global save owners before world preparation. No title-specific detached world constructor or atomic owner swap exists yet, so live MM6 SaveLoad stays closed.

Synthetic native tests use nonzero actor PIDs in both indoor and outdoor active deltas and reject out-of-range or wrong-type PIDs after all 67 base reads without changing caller owners. This does not claim an MM6 launch or post-load visual proof.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Partial Stage Coordinator](MM6-F4-Partial-Stage-Coordinator.md), [World Save Runtime](MM6-F4-World-Save-Runtime.md).
