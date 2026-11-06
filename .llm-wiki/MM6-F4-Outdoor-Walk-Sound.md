---
type: synthesis
up: "[[MM6-F4-Outdoor-Tile-Selectors]]"
tags: [mm6, f4, outdoor, terrain, audio]
---

# MM6 F4 Outdoor Walk Sound

The [public packet](../project/MM6_F4_OUTDOOR_WALK_SOUND.md) records an F3-identity-matched original-executable trace of MM6 outdoor walking and running sound selection.

The original movement caller obtains a tile from the authored selector base and dispatches on the tile-table's raw tileset field. A private read-only reconciliation covered all 28 entries. The inherited runtime's normalized MM7 enum merges MM6 terrain and road types, so `TileData` now retains raw source tileset and variant IDs and an MM6-only movement route uses the source tileset for sound. MM7 retains the former normalized lookup. Unsupported raw IDs fail closed with no new sound. Focused source-backed tests cover all 28 entries by sound family, raw field retention, malformed IDs and MM7 isolation; linked headless and audible checks are pending.

The original seasonal/transition policy is still open. Raw identity retention supports future title-specific modeling, but no wider season or shoreline behavior is inferred from the walking-sound dispatch. No original executable bytes, private tile records or audio data are tracked.

See also: [Outdoor Tile Selectors](MM6-F4-Outdoor-Tile-Selectors.md), [First Map Preflight](MM6-F4-First-Map-Preflight.md).
