# MM6 F4 terrain identity and season schema audit

Status: **raw global tile ID is the required MM6 art identity; original seasonal remapping is not yet established** (2026-11-06).

The retained MM6 tile table has **882 rows** and **24 raw tileset values**, including its invalid sentinel. Its pair of raw tileset and variant values is not a unique art key: **59** such pairs occur more than once, with a maximum multiplicity of **77**. The shared MM7 `Tileset` enum also collapses several distinct MM6 raw groups to dirt or another broad category. A future MM6 terrain model must therefore retain the global row ID and map-local selector byte as primary source identity. Raw tileset and variant fields can classify behavior, but cannot reconstruct an exact tile on their own. These are aggregate checks of ignored F3-retained data; no source records are reproduced here.

The installed local MM6 executable was read only after its SHA-256 matched the accepted F3 capture. Its terrain accessor at `0x46e1f0` reads the map-local byte and adds the selected serialized base for local IDs 90–233. That accessor has no month input or month branch. This verifies the direct lookup path, **not** that no other original routine edits the map, tile table, palette or rendered texture by season. The original winter, spring and autumn display policy remains open.

A follow-up whole-PE xref sweep found the terrain object's local-grid pointer (`+0xb8`) used for buffer allocation, a 16,384-byte archive copy on load, a corresponding archive write, and raw accessor reads. The inspected accessor region has no grid-byte writer. This narrows a possible terrain-ID remap to an indirect alias or a different display path; it does not eliminate either. The original rendering texture resolver and any calendar-driven texture or palette choice have not been closed. The precise blocker for removing the shared MM7 season transform is therefore **absence of a proved original MM6 display policy**, not a missing tile-table row. Detailed disassembly remains ignored locally.

The current `OutdoorTerrain::changeSeason` uses the shared MM7 season and transition machinery. The MM6 branch now preserves authored global IDs when that machinery leaves the normalized tileset unchanged; the current code still maps changed grass through `TileTable::tileId(normalizedTileset, normalizedVariant)`. That lookup selects the first normalized pair and cannot preserve a source-specific MM6 grass variant or prove a source-specific snow/dirt replacement. Recalculating transitions through the normalized classification has the same limitation. The existing all-map/month texture-budget check proves valid resource allocation, not original seasonal fidelity.

## Next evidence and implementation gate

1. Close indirect aliases of the original outdoor grid pointer and trace the original terrain texture resolver for calendar-dependent choices, or perform a controlled original-runtime month comparison. Keep all detailed traces and captures ignored locally.
2. Record the exact original policy separately for terrain IDs, shoreline/transition art, and vegetation sprites. Do not infer one from another.
3. If MM6 changes terrain by season, add a title-specific mapping keyed by the original global tile ID (and map selector where needed), with explicit ambiguity/error handling. Keep MM7's current normalized path separate. If the original retains terrain year-round, make MM6's terrain projection a direct copy of authored IDs.
4. Verify all 15 outdoor maps across 12 months for bounds, source identity where unchanged, exact changed-art expectations where known, hidden rendered frames for representative regions and seasons, and an MM7 control.

The current code and tests remain unchanged by this audit. MM6 campaign play, shoreline fidelity and durable save/reload are separate gates.

See [outdoor tile selectors](MM6_F4_OUTDOOR_TILE_SELECTORS.md), [offscreen frame capture](MM6_F4_OFFSCREEN_FRAME_CAPTURE.md), and [F4 build graph](MM6_F4_BUILD_GRAPH.md).
