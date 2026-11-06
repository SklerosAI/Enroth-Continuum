---
type: synthesis
up: "[[MM6-F4-First-Map-Preflight]]"
related: ["[[MM6-F4-Offscreen-Render-Capture]]"]
tags: [mm6, f4, outdoor, terrain, headless]
---

# MM6 F4 Outdoor Tile Selectors

The [public packet](../project/MM6_F4_OUTDOOR_TILE_SELECTORS.md) records the original-executable trace and the title-specific selector reconstruction.

The earlier statement that the original first outdoor map contains 209 tile-zero cells is superseded. Its used terrain grid has no literal local tile ID zero. The inherited decoder discarded the four serialized base tile IDs and substituted bases through an MM7 tileset enum that collapses MM6 terrain and road types. That led 209 cells to the `pending` placeholder. The F3-identity-matched original executable reads the selected record's base field and adds the local offset. MM6 reconstruction now follows that field, validates its table span and each resulting ID, and leaves MM7's lookup unchanged.

An offline replay of all 15 retained outdoor maps found zero blank/`pending` textures across 241,935 used cells with the authored bases. The focused GameTest checks first-map before/after mapping, neutral summer no-`pending`, all-map table bounds, malformed MM6 bases and MM7 title isolation. Its first linked attempt exhausted the thread stack with large local fixtures; the fixtures were moved to heap ownership and the monitored test passed 1/1 across 353 samples without a window/focus change. The MM7 control passed 1/1 across 48 samples. No original map, bitmap, executable bytes, detailed asset inventory or private screenshot is tracked.

The inherited seasonal transform projected MM6-only terrain and road variants through generic dirt. A read-only model of its variant remap, before transition recalculation, changed 1,230 first-map cells at neutral summer month without producing `pending`. The MM6 branch now preserves an authored global tile ID whenever the current season rule leaves its normalized tileset unchanged, including after the shared transition pass. MM7 retains its prior transform. The monitored headless test passed 1/1 over 179 samples for neutral-summer identity across all 15 maps, winter preservation of MM6-only terrain/roads, grass change and MM7 isolation; the MM7 control passed 1/1 over 40 samples, with no visible window/focus change. The selector-only hidden recapture removed the cyan/red placeholder but exposed black ground because MM6 land and water were allocated to one texture unit. Full original MM6 season semantics, changed-grass art, walking sounds and a correct hidden frame remain unverified. A future MM6 tile schema should retain raw terrain/variant identity for broader fidelity work. Normal save/chest guards remain closed.

See also: [First Map Preflight](MM6-F4-First-Map-Preflight.md), [Offscreen Render Capture](MM6-F4-Offscreen-Render-Capture.md).
