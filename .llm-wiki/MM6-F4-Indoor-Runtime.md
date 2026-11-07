---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[Source-MM6-World-F2-Closure]]"]
tags: [mm6, f4, indoor, runtime]
---

# MM6 F4 Indoor Runtime

The [implementation packet](../project/MM6_INDOOR_RUNTIME_F4.md) maps MM6 BLV/DLV sections into the existing `IndoorLocation` renderer state. It remains an offline seam, with no MM6 launch or graphical validation. The [F3 gate](MM6-F3-Read-Only-Preflight.md) established that the 52 retained pairs match the owner's original content; this work reads only those ignored fixtures.

The adapter projects faces, variable face arrays, rooms, decorations, lights, BSP nodes, spawn points, outlines, actors, ground items, chests, 200 door records and visibility. It checks references before live reconstruction and retains decoded DLV bytes so unknown fields survive until the MM6 save writer patches verified changes. The two known BLV checksum anomalies use a filename-scoped fallback with wrapper, decoded-length and full-layout checks. MM7's loader remains in its original branch.

Five native indoor tests pass, including all 52 retained base/delta pairs, byte-exact no-change encoding and targeted actor/item/chest/door edits. The changed-state encoder captures supported live fields from the active indoor location, starting with its current raw sidecar so separate map-variable edits survive. It rejects changed record counts, negative actor HP, chest-grid changes and door geometry changes. SaveLoad still needs to call it in the `.ec6` transaction. This proves bounded projection and selected wire edits, not room rendering, moving doors, chest-grid behavior, event effects, reset policy or durable persistence. See the [F4 offline seam handoff](MM6-F4-Offline-Seams.md) and [world closure](Source-MM6-World-F2-Closure.md).

The [detached v2 indoor plan](../project/MM6_INDOOR_RUNTIME_F4.md#detached-v2-indoor-world-plan) now reprojects a validated BLV, rechecks DLV and CHST sidecar counts and bounds all door references before any live map or renderer owner is touched. It rejects structural DLV/CHST door disagreement. The represented v2 session preflight now requires this plan for indoor slots before primary/backup selection. Linked indoor/raw checks passed **12/12**, with focused coordinator assertions **4/4**. Live reconstruction, resources, PID identity and IFOV replay remain separate gates. This supersedes the earlier unjoined-plan note.
