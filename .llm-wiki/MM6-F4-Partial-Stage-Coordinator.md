---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-World-Save-Runtime]]", "[[MM6-F4-Map-Save-Fields]]"]
tags: [mm6, f4, save, staging]
---

# MM6 F4 Partial Stage Coordinator

The [coordinator packet](../project/MM6_F4_PARTIAL_STAGE_COORDINATOR.md) records one detached transaction boundary over the sixteen currently known `.ec6` sections, 67 MM6 map pairs, NPC/map buffers and timer/overlays.

`stageKnownWorldAndVisuals()` decodes the complete known-section candidate and cross-field joins, validates each saved delta against its trusted MM6 base and item templates, then retains the active map as an owned typed indoor base/delta or outdoor base/delta. It copy-stages NPC groups/records and all map buffers. Its final step stages a paused simulation timer and 50 overlays against the loaded table size. A late overlay rejection after the world copies, a malformed final delta, or a foreign title yields no result and leaves caller state unchanged. The candidate is returned only after every step succeeds.

The [active-world PID preflight](MM6-F4-Active-World-PID-Restore.md) bounds each active overlay actor PID against the active **saved** delta after the 67-map pass. The retained typed candidate makes that same decoded active state available to a future independent world reconstruction; it does not reconstruct the live world. `stageKnownSession()` now composes the represented core, durable, resistance, buff, hireling, held-item and recovery adapters on one detached Party after the world/visual candidate succeeds. `captureKnownPartyFields()` merges their live capture adapters into a known-section seed. Seven focused native world-save tests passed using a manually linked arm64 unit binary, including nonzero inventory and held items and late HELD/recovery rejection without caller mutation. This remains **partial**: MM6 SHOP/guild/bounty state, Character bonus bytes and beacons, movement/flight, pending actions and rest/turn policy are unrepresented; saved actor index does not prove post-load identity; HEAD has no loaded-resource check here; and no all-owner commit exists. The inherited chest generator is separately gated. Keep MM6 `SaveLoad.cpp` closed.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [World Save Runtime](MM6-F4-World-Save-Runtime.md), [Map Save Fields](MM6-F4-Map-Save-Fields.md), [Active World PID Restore](MM6-F4-Active-World-PID-Restore.md).
