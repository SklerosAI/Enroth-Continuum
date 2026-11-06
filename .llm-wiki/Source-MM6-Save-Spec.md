---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, maps, saves]
---

# MM6 Save and Map Specification Source

This F2 source defines the observed MM6 map-delta boundaries and a separate, versioned `.ec6` save contract.

Retained baseline deltas support the boundaries of the opening, persistent map region, and final tail, but those fixed regions are zero-filled; their nonzero field meanings remain unverified. External readers offer leads for visit time, sky and weather fields, while some tail bytes remain unknown. The inherited MM7 map header, reset path, direct save write and load-menu filter cannot be reused by changing an extension. A future serializer must preserve unknown bytes, survive failed writes, and prove map-state round trips. Native `.mm6` save compatibility has not been established.

The separate [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md) and [roundtrip probe](../scripts/mm6_probe_world_roundtrip.py) now show byte-exact section reassembly for all retained deltas and synthetic nonzero preservation and malformed-input rejection in the offline Python checker. This tightens the wire-boundary evidence, while leaving MM6 C++ reconstruction, reset and field semantics, and an end-to-end `.ec6` save/reload unverified.

The [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md) records the original F2 ambiguity: all retained baseline chest grids were zero, so it could not distinguish 81 cells plus a tail from 140 cells. A later [original-executable save-path trace](MM6-F4-Chest-Grid-Wire.md) proves the accepted MM6 writer copies all 140 signed cells to changed delta output. Live grid projection and save/reload remain gated.

Authority: [project/MM6_SAVE_SPEC.md](../project/MM6_SAVE_SPEC.md), reviewed 2026-11-05. Its exact wire layout and source-consumer table remain the implementation reference; this wiki contains no decoded map bytes or saved-game contents.

See also: [MM6 World Field Map F2 Source](Source-MM6-World-Field-Map-F2.md), [MM6 World F2 Closure Source](Source-MM6-World-F2-Closure.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Runtime Isolation](Runtime-Isolation.md).
