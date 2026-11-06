---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, maps, persistence]
---

# MM6 World Field Map F2 Source

The [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md) and tracked [field probe](../scripts/mm6_probe_world_fields.py) bound record sections across the retained indoor and outdoor delta companions and compare them with pinned primary editor readers and current C++ consumers. The result is **retained-byte and source-hypothesis evidence**, not verified gameplay field meaning. Creature, ground-item, chest and indoor-door records have bounded widths; equal widths with an MM7 record do not establish equal semantics. Detailed offsets, map fingerprints and record changes stay in ignored local reports.

**Supersession:** The F2 retained baselines could not distinguish a 140-cell grid from the pinned editor's 81-cell-plus-tail reading, because all 1,340 chest grid regions were zero. The later [original-executable placement and save-path traces](MM6-F4-Chest-Grid-Wire.md) establish that the accepted MM6 runtime and serialized delta both use the full 280 bytes as 140 signed grid cells. `dchest.bin` separately declares 14×10 layout metadata. Live chest projection, changed-state save/reload and MM7 isolation remain open.

Other zero-filled baseline regions include the reset header, 200 persistent map bytes and 56-byte tail. A pinned editor supplies candidate field labels, but the map-bit reader does not copy the bytes into its advertised field. Nonzero changes, title-specific C++ decode/reconstruction and graphical save/reload are still needed before semantic claims.

Authority: [project/MM6_WORLD_FIELD_MAP_F2.md](../project/MM6_WORLD_FIELD_MAP_F2.md), read with the [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md), [save specification](../project/MM6_SAVE_SPEC.md) and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). F2 is frozen for F3; nonzero field meaning and runtime saves remain open.

See also: [MM6 World F2 Closure Source](Source-MM6-World-F2-Closure.md), [MM6 Save and Map Specification Source](Source-MM6-Save-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
