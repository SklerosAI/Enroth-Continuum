# MM6 F4 outdoor tile selectors

Status: **MM6-only selector reconstruction staged; linked headless and hidden graphical acceptance pending** (2026-11-06).

The earlier offscreen diagnosis that the original first outdoor map contains 209 tile-zero cells is superseded. A read-only decode of the retained map found no literal local tile ID zero in its used 127×127 terrain grid. The 209 `pending` textures came from inherited reconstruction: it ignored the map's four serialized base tile IDs, converted their tile-set labels through an MM7 enum that collapses MM6 terrain and road types to dirt, then added local offsets to that substituted base. The original local IDs were valid; the reconstruction was not faithful.

The staged original MM6 executable was rehashed against the accepted F3 SHA-256 before inspection. Its terrain accessors at `0x46e1dd` and `0x46e225` read the signed 16-bit base field of a selected four-byte terrain record, then add the local tile offset. This establishes the base field as the map's intended global tile index source. Exact executable bytes and private map records remain local and ignored.

`OutdoorLocation::Load` now passes the title to outdoor reconstruction. MM6 uses each serialized base directly, requiring a nonzero 36-tile span within the loaded tile table and checking every reconstructed tile ID before storage. MM7 retains its existing base lookup through the normalized tileset. The change does not alter tile-table contents or substitute a new bitmap for `pending`.

A read-only replay of all **15** retained MM6 outdoor maps checked **241,935** used terrain cells against the retained tile-table size and found **zero** blank or `pending` textures with serialized bases. For the first map, the inherited mapping produced **209** `pending` cells and the corrected pre-season mapping produced **zero**. A headless GameTest stages these comparisons, verifies a separate neutral summer-season no-`pending` result, rejects zero/out-of-range MM6 selectors, and checks that the MM7 branch ignores those MM6 selector fields. Linked execution is pending integration.

The seasonal path remains a fidelity gap. A read-only model of the current variant remap, before transition recalculation, replaced **1,230** first-map cells at neutral summer month because the shared `TileData` tileset enum collapses MM6-only terrain and road types to generic dirt. It did not reintroduce `pending` in that model, but the artwork is not faithful. Original MM6 seasonal rendering has not been traced fully; this patch does not change the season transform. A hidden OpenGL recapture and season/art audit remain required before claiming correct terrain appearance or playable MM6.

The map still relies on the existing out-of-range local-tile sentinel mapping; this packet establishes only the selector base for valid mapped cells. Normal save/chest guards remain closed.
