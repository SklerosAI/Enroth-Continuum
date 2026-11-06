---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, maps, persistence]
---

# MM6 World Field Map F2 Source

The [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md) and tracked [field probe](../scripts/mm6_probe_world_fields.py) bound record sections across the retained indoor and outdoor delta companions and compare them with pinned primary editor readers and current C++ consumers. The result is **retained-byte and source-hypothesis evidence**, not verified gameplay field meaning. Creature, ground-item, chest and indoor-door records have bounded widths; equal widths with an MM7 record do not establish equal semantics. Detailed offsets, map fingerprints and record changes stay in ignored local reports.

**Supersession:** The MM6 delta chest's final 280 bytes cannot yet be assigned a 14×10 grid. Separate `dchest.bin` metadata declares a 14×10 layout, while a pinned editor reader splits the delta bytes into 81 grid shorts plus 118 opaque bytes; another candidate reads 140 shorts. Both candidate subregions are zero in all 1,340 retained baseline chests. Neither interpretation is confirmed. Preserve the 280 bytes losslessly and seek changed-state evidence before coding chest placement or converting to the inherited MM7 grid. The earlier 14×10 *delta* assertion is withdrawn in the [save specification](../project/MM6_SAVE_SPEC.md) and [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md).

Other zero-filled baseline regions include the reset header, 200 persistent map bytes and 56-byte tail. A pinned editor supplies candidate field labels, but the map-bit reader does not copy the bytes into its advertised field. Nonzero changes, title-specific C++ decode/reconstruction and graphical save/reload are still needed before semantic claims.

Authority: [project/MM6_WORLD_FIELD_MAP_F2.md](../project/MM6_WORLD_FIELD_MAP_F2.md), read with the [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md), [save specification](../project/MM6_SAVE_SPEC.md) and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). F2 is frozen for F3; nonzero field meaning and runtime saves remain open.

See also: [MM6 World F2 Closure Source](Source-MM6-World-F2-Closure.md), [MM6 Save and Map Specification Source](Source-MM6-Save-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
