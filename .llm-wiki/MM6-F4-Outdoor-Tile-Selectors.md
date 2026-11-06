---
type: synthesis
up: "[[MM6-F4-First-Map-Preflight]]"
related: ["[[MM6-F4-Offscreen-Render-Capture]]"]
tags: [mm6, f4, outdoor, terrain, headless]
---

# MM6 F4 Outdoor Tile Selectors

The [public packet](../project/MM6_F4_OUTDOOR_TILE_SELECTORS.md) records the original-executable trace and the title-specific selector reconstruction.

The earlier statement that the original first outdoor map contains 209 tile-zero cells is superseded. Its used terrain grid has no literal local tile ID zero. The inherited decoder discarded the four serialized base tile IDs and substituted bases through an MM7 tileset enum that collapses MM6 terrain and road types. That led 209 cells to the `pending` placeholder. The F3-identity-matched original executable reads the selected record's base field and adds the local offset. MM6 reconstruction now follows that field, validates its table span and each resulting ID, and leaves MM7's lookup unchanged.

An offline replay of all 15 retained outdoor maps found zero blank/`pending` textures across 241,935 used cells with the authored bases. The focused GameTest checks first-map before/after mapping, neutral summer no-`pending`, all-map table bounds, malformed MM6 bases and MM7 title isolation; its linked result is pending integration. No original map, bitmap, executable bytes, detailed asset inventory or private screenshot is tracked.

The existing seasonal transform still projects MM6-only terrain and road variants through generic dirt. A read-only model of its variant remap, before transition recalculation, changed 1,230 first-map cells at neutral summer month without producing `pending`. Its art fidelity and the corrected hidden OpenGL frame remain unverified. Normal save/chest guards remain closed.

See also: [First Map Preflight](MM6-F4-First-Map-Preflight.md), [Offscreen Render Capture](MM6-F4-Offscreen-Render-Capture.md).
