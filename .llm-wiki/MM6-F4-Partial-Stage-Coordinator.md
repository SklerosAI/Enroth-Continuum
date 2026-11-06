---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-World-Save-Runtime]]", "[[MM6-F4-Map-Save-Fields]]"]
tags: [mm6, f4, save, staging]
---

# MM6 F4 Partial Stage Coordinator

The [coordinator packet](../project/MM6_F4_PARTIAL_STAGE_COORDINATOR.md) records one detached transaction boundary over the sixteen currently known `.ec6` sections, 67 MM6 map pairs, NPC/map buffers and timer/overlays.

`stageKnownWorldAndVisuals()` decodes the complete known-section candidate and cross-field joins, validates each saved delta against its trusted MM6 base and item templates, then copy-stages NPC groups/records and all map buffers. Its final step stages a paused simulation timer and 50 overlays against the loaded table size. A late overlay rejection after the world copies, a malformed final delta, or a foreign title yields no result and leaves caller state unchanged. The candidate is returned only after every step succeeds.

This remains **partial**. The Party field-family adapters are not composed on one Party copy; active actors/chests are not reconstructed; overlay PIDs are not rebound; the header is not checked against loaded map resources in this pure seam; and the mandatory manifest still lacks several owners. The inherited chest generator is separately gated. Keep MM6 `SaveLoad.cpp` closed until all owners can be staged and committed without an intervening failure.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [World Save Runtime](MM6-F4-World-Save-Runtime.md), [Map Save Fields](MM6-F4-Map-Save-Fields.md).
