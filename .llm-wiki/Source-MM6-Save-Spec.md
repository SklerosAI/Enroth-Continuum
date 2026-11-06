---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, maps, saves]
---

# MM6 Save and Map Specification Source

This F2 source defines the observed MM6 map-delta boundaries and a separate, versioned `.ec6` save contract.

Retained baseline deltas support the boundaries of the opening, persistent map region, and final tail, but those fixed regions are zero-filled; their nonzero field meanings remain unverified. External readers offer leads for visit time, sky and weather fields, while some tail bytes remain unknown. The inherited MM7 map header, reset path, direct save write and load-menu filter cannot be reused by changing an extension. A future serializer must preserve unknown bytes, survive failed writes, and prove map-state round trips. Native `.mm6` save compatibility has not been established.

Authority: [project/MM6_SAVE_SPEC.md](../project/MM6_SAVE_SPEC.md), reviewed 2026-11-05. Its exact wire layout and source-consumer table remain the implementation reference; this wiki contains no decoded map bytes or saved-game contents.

See also: [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Runtime Isolation](Runtime-Isolation.md).
