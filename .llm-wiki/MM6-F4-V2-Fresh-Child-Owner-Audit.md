---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]", "[[MM6-F4-Persistent-Event-Publication]]", "[[MM6-F4-Post-Rebuild-PID-Slots]]"]
tags: [mm6, f4, save, reload, handoff]
---

# MM6 F4 V2 Fresh-Child Owner Audit

The [public owner packet](../project/MM6_F4_V2_FRESH_CHILD_OWNER_AUDIT.md) inventories the process owners that a native v2 fresh-child loader must validate before first input. The current v2 two-process check stops after read-only detached stage; the earlier v1 callback proves the Main Menu → `gameLoop()` control path only. `gameLoop()` has no post-`DoPrepareWorld(true)` admission callback before `MessageLoopWithWait()`. That ordering is the immediate integration gate.

The test-only hook is now directly after `DoPrepareWorld(bLoading, 1)` and before timer unpause. A disposable test load callback can hold `ScopedPostWorldAdmissionForTest` on the game thread; the hook consumes its callback once and propagates a final-owner refusal before input. Unarmed v1/MM7 paths remain unchanged. Four linked unit tests passed; a disabled v1 headless control compiled but awaits its private restart fixture in this isolated worktree. The hook does not itself validate or publish v2 owners.

The integrated test-only inspector now names blockers for global NPC/static resources, BEAC/IFOV, map event/timer triggers, final chest/actor/sprite identity, visible UI/cursor resources, actions and the missing post-world hook. It also rechecks `UIST`, inactive VISU and no-beacon policy on its detached candidate. These blockers are not implemented live owners. `DoPrepareWorld` resets mouse/windows/resources, rebuilds the map, may respawn or generate chests, rewrites NPC IDs and executes `onMapLoad` before input. Pre-map detached equality cannot stand in for a final-world check. The inspector remains blocked; normal MM6 SaveLoad and Atlas Play remain closed.

No private source bytes, captures or saves are included. See [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Persistent Event Publication](MM6-F4-Persistent-Event-Publication.md), and [Post-Rebuild PID Slots](MM6-F4-Post-Rebuild-PID-Slots.md).
