---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[Source-MM6-World-F2-Closure]]"]
tags: [mm6, f4, indoor, runtime]
---

# MM6 F4 Indoor Runtime

The [implementation packet](../project/MM6_INDOOR_RUNTIME_F4.md) maps MM6 BLV/DLV sections into the existing `IndoorLocation` renderer state. It remains an offline seam, with no MM6 launch or graphical validation. The [F3 gate](MM6-F3-Read-Only-Preflight.md) established that the 52 retained pairs match the owner's original content; this work reads only those ignored fixtures.

The adapter projects faces, variable face arrays, rooms, decorations, lights, BSP nodes, spawn points, outlines, actors, ground items, chests, 200 door records and visibility. It checks references before live reconstruction and retains decoded DLV bytes so unknown fields survive until the MM6 save writer patches verified changes. The two known BLV checksum anomalies use a filename-scoped fallback with wrapper, decoded-length and full-layout checks. MM7's loader remains in its original branch.

Four native indoor tests pass, including all 52 retained base/delta pairs and byte-exact round trips. This proves the bounded projection and source shape, not room rendering, moving doors, chest-grid behavior, event variables, reset policy or persistence. The live `.ec6` writer still needs to apply changed runtime fields into the retained wire; see the [F4 offline seam handoff](MM6-F4-Offline-Seams.md) and [world closure](Source-MM6-World-F2-Closure.md).
