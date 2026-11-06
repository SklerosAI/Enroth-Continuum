---
type: synthesis
up: "[[MM6-F4-Outdoor-Tile-Selectors]]"
related: ["[[MM6-F4-Offscreen-Render-Capture]]"]
tags: [mm6, f4, outdoor, terrain, schema]
---

# MM6 F4 Terrain Schema Audit

The [public audit packet](../project/MM6_F4_TERRAIN_SCHEMA_AUDIT.md) separates exact MM6 terrain identity from the still unproved seasonal display policy.

The retained MM6 tile table has 882 rows in 24 raw tileset groups. Raw tileset plus variant is nonunique: 59 pairs repeat, so a global tile row ID remains the primary art key. The shared MM7 tileset classification is useful for broad behavior but loses several MM6 source groups. The F3-matched original executable's direct terrain accessor adds the map's authored selector base to its local tile byte without reading a month; other original paths could still change the grid or display and must be traced before concluding that MM6 terrain has no seasons.

The current MM6 `changeSeason` preserves authored IDs only where the shared season classification stays unchanged. Changed grass and recalculated transitions still use normalized MM7 lookups. A valid hidden frame and the all-map/month texture-budget test do not establish original winter/spring/autumn appearance. Resolve original terrain, shoreline and sprite policies independently; then use an MM6-specific global-ID mapping or direct authored-ID copy, depending on the evidence. Keep MM7's existing route isolated.

See also: [Outdoor Tile Selectors](MM6-F4-Outdoor-Tile-Selectors.md), [Offscreen Render Capture](MM6-F4-Offscreen-Render-Capture.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
