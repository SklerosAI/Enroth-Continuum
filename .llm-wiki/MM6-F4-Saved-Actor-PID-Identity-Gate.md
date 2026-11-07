---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Active-World-PID-Restore]]", "[[MM6-F4-Raw-Save-Runtime]]"]
tags: [mm6, f4, save, world, pid]
---

# MM6 F4 Saved Actor PID Identity Gate

The [source audit](../project/MM6_F4_SAVED_ACTOR_PID_IDENTITY_GATE.md) explains why an actor PID bounded by the saved DLV/DDM count is not yet a safe target after map reconstruction.

Indoor and outdoor MM6 reconstruction initially keep actor order and assign `id = index`. Initial/timed/invalid-delta respawn can substitute fresh actors; loader spawn and debug paths can add or remove actors; `OnMapReload` events can summon actors and `AllocateActor` can reuse a removed index. Numeric bounds therefore do not prove post-load identity.

The detached MM6-only `requireNoActiveSavedActorOverlays` policy refuses active `VISU` slots before publication and keeps inactive slots unchanged. It is now required inside v2 primary/backup preflight, so an active-overlay primary can fall back to an inactive backup without disk repair. Focused policy and coordinator checks passed **4/4**. Live SaveLoad remains guarded; this supersedes the unjoined-policy note.

The next gate is a detached selected-delta and post-reload actor-identity proof, including respawn, initialization, debug and event effects. Only then should the all-owner transaction publish an active actor overlay, followed by visible save/relaunch checks.

See also: [Active World PID Restore](MM6-F4-Active-World-PID-Restore.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
