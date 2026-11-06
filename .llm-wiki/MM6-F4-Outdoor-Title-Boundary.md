---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, outdoor, isolation]
---

# MM6 F4 Outdoor Title Boundary

The [outdoor boundary packet](../project/MM6_F4_OUTDOOR_TITLE_BOUNDARY.md) records MM7-only map table paths that now reject MM6 use before reading the wrong route or drawing fog RNG.

MM6 border travel currently returns no destination. The [original border trace](MM6-F4-Outdoor-Border-Trace.md) establishes a geometric neighbor grid, timing and edge-offset arrival shape, but party-state eligibility remains untyped and there is no runtime adapter. MM6 random fog does not use the MM7 map-index table, and MM7's underwater-map classifier does not run in the MM6 outdoor initializer, central world transition or indoor loader. The loader assertion checks the MM6 map table's `.odm` suffix rather than an MM7 enum range. The MM7 route and fog paths retain their prior behavior. Fog probabilities and underwater behavior remain open, and there has been no MM6 graphical check.

The shared event interpreter now gates three MM7 campaign exceptions by title. Map transitions still call autosave before committing a cross-map move, so the incomplete [`.ec6` transaction](MM6-F4-Save-Transaction-Checklist.md) also blocks that travel path.

See also: [MM6 F4 Outdoor Border Trace](MM6-F4-Outdoor-Border-Trace.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 World Save Runtime](MM6-F4-World-Save-Runtime.md), [Runtime Isolation](Runtime-Isolation.md).
