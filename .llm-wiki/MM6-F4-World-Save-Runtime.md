---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, save, maps, npcs]
---

# MM6 F4 World Save Runtime

The [world-state adapter packet](../project/MM6_F4_WORLD_SAVE_RUNTIME.md) connects current `NPCStats` records/groups and all 67 `pMapDeltas` buffers to the partial sixteen-section `.ec6` assembly. Capture requires a supplied encoded delta for the active map and replaces the stale saved copy. Stage returns detached NPC and map copies and preserves original NPC tables; no global state is assigned. Both directions reject foreign titles and mismatched map sets.

`captureKnownWorldFromActiveMap` now obtains that delta through the real bounded MM6 indoor/outdoor changed-state writer after checking title and map type. Indoor capture also matches the loaded filename to `HEAD`. The writers return new bytes and reject unsupported live changes; the adapter does not compact objects or mutate map globals. The outdoor runtime lacks its own filename, so the live current-map-table/`HEAD` join remains a transaction gate.

The arm64 MM6/MM7/unit build passed and three focused tests passed, including a synthetic active indoor sidecar through the real writer with nonzero respawn values. The source returns a candidate without changing the NPC or map globals. The outdoor route still needs a live-runtime F5 check.

This is below the [private base-map validation](MM6-F4-Map-Save-Fields.md) and [read-only disk preflight](MM6-F4-Atomic-Save-File.md). The active delta must come from the bounded MM6 indoor/outdoor writer, and every candidate map must pass the private 67-name/base/item check before a live commit. Party, services, timers, remaining campaign fields and atomic global assignment are still open. Keep `SaveLoad.cpp` guarded and use the [remaining-field register](MM6-F4-Save-Remainder.md) before extending the transaction.
