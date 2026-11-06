# MM6 world and persistence F2 closure packet

Status: **bounded wire-section evidence, not field semantics or a working save path**. Reviewed 2026-11-05 on `mm6-mac-implementation` from the retained ignored MM6 map references and current C++ source. This packet is for integration with [MM6_SAVE_SPEC.md](MM6_SAVE_SPEC.md); it does not advance the F2 or F3 gate by itself.

## Reproducible section-preservation check

`scripts/mm6_probe_world_roundtrip.py` reads the 52 retained indoor `.dlv` and 15 outdoor `.ddm` companions, after the existing base-layout probes establish their paired `.blv`/`.odm` structure. It uses strict entry decoding for deltas, splits each decoded delta into bounded wire sections, reassembles those sections byte for byte, and rejects count overflow, truncation and an inconsistent indoor door-array span. The detailed per-map hashes and section positions are kept only in ignored `local-game-data/mm6/research-2026-11-05/world-roundtrip-probe.json`.

The probe passed 67 of 67 byte-exact **section reassemblies**, 268 synthetic nonzero sentinel checks across reset-header, visibility, persistent-byte and tail regions, and 268 malformed-input rejections. All 67 shipped baseline reset headers, persistent-byte blocks and tails remain zero-filled. These checks establish section boundaries and preservation of opaque bytes in this Python probe. They **do not** prove any field's gameplay meaning, any C++ MM6 reader/writer, native `.mm6` save compatibility, or an end-to-end `.ec6` save/reload.

## Field, consumer and acceptance matrix

| MM6 wire region | Current C++ consumer / gap | F4/F5 obligation |
| --- | --- | --- |
| Base indoor `.blv` | `IndoorLocation::Load` in `src/Engine/Graphics/Indoor.cpp` still deserializes `IndoorLocation_MM7`; only the offline MM6 indoor probe understands the shorter retained layout. | Add a bounded MM6 indoor decoder and reconstruct vertices, faces, rooms, sprites, lights, spawns and door links across all 52 maps; inspect geometry and collision live. |
| Base outdoor `.odm` | `OutdoorLocation::Load` branches to `deserializeMM6Outdoor` in `src/Engine/Snapshots/MM6OutdoorSnapshot.cpp`, then reconstructs shared structures. | Verify all 15 maps' terrain, models, decorations, encounters, collision and event coordinates live; preserve unknown base fields when writing derived state. |
| Reset header (8 bytes) | Indoor and outdoor loaders compare `lastRespawnDay`, choose initial/timed reset, restore shipped deltas and update `respawnCount`; both paths currently read `LocationHeader_MM7` from an MM7 delta. | Define MM6 widths, sentinel and refill schedule; test first visit, early return, timed reset, and save-loaded re-entry without losing protected state. |
| Visibility (875 indoor; 1,936 outdoor bytes) | `CompositeSnapshots.cpp` reconstructs indoor outlines and two outdoor revealed-cell arrays from MM7 snapshots; timed reset retains prior visibility. | Translate MM6 bits without shift/reversal, reveal known cells, leave/re-enter and save/reload; check map view visually. |
| Creature records (548 bytes each) | MM7 `CompositeSnapshots.cpp` reconstructs `pActors` from `Actor_MM7`; the MM6 record width and fields differ. | Decode active MM6 actor fields, spawn/combat/death and refill; preserve unknown bytes and verify state across transitions and save/reload. |
| Ground item records (100 bytes each) | MM7 snapshot reconstructs `pSpriteObjects` and recalculates item sprite IDs. | Preserve MM6 item ID, position, lifetime and flags; test pickup/drop, revisit and persistence without MM7 item-range assumptions. |
| Chest records (4,204 bytes each) | MM7 `CompositeSnapshots.cpp` reconstructs `vChests`; `ChestTable.cpp` uses an MM7 9 by 9 layout. MM6 `dchest.bin` declares 14 by 10 layout metadata, but the retained delta's final 280 bytes have unresolved grid/tail semantics; see [world field map](MM6_WORLD_FIELD_MAP_F2.md). | Preserve all MM6 chest bytes; determine the grid from nonzero changed state, then test loot/removal, transitions, reload and out-of-grid placement without altering MM7 chests. |

**F4 supersession:** The accepted original executable's [save-path trace](MM6_F4_CHEST_GRID_WIRE.md) proves all 280 final chest bytes are serialized as 140 signed grid cells. This resolves the F2 row's grid/tail ambiguity, while live projection and changed-state save/reload remain guarded.
| Indoor door records and variable arrays | `CompositeSnapshots.cpp` repairs door pointers, vertices, face texture deltas and normals after MM7 snapshot reconstruction. The retained MM6 fixed record width is also 80 bytes, but that does not prove matching field semantics. | Bound all 200 slots and base-header array span, resolve vertex/face/sector references, operate closed/open/moving doors and verify geometry after save/reload. |
| Persistent map bytes (200) | `CompositeSnapshots.cpp` maps MM7's 75 map plus 125 decoration bytes into `engine->_persistentVariables`; `EvtVariables.cpp` lacks an MM6 variable dispatch. | Preserve all 200 bytes, reconcile MM6 event ID 105–204 map-byte addressing and other state IDs, then test boundary variables across map exit and reload. Do not assume the MM7 split. |
| Tail (56 bytes) | MM7 snapshot code reads/writes visit time and weather through `MapWeather_MM7`; the MM6 tail labels remain partly inferred from editor readers. | Preserve opaque bytes, prove nonzero time/sky/fog fields against the shipped engine's behavior in F3/F5, and validate day/night and reset effects. |
| Whole save/profile | `SaveLoad.cpp` copies baseline deltas, serializes the active map via MM7 delta types and writes `SaveGame_MM7` directly. `UISaveLoad.cpp` filters `.mm7`; `PathResolver.cpp` gives MM6 a distinct profile root. | Define versioned `.ec6`, reject cross-title/unsupported headers before state mutation, write atomically with recovery, and validate manual, auto and quick save/reload plus thumbnails. Keep MM7's known-good profile and app unchanged. |

## Integration order and negative checks

1. Resolve MM6 base/companion record mapping and persistent-byte semantics before translating to shared runtime structures. Keep unknown bytes in a lossless sidecar or a versioned MM6 structure. Zero-filled shipped fixed regions must never stand in for nonzero tests.
2. Implement an MM6 delta decoder and encoder with strict bounds and source fingerprints. Reuse the offline 67-map corpus and synthetic sentinels, but add C++ decode → reconstruct → modify → encode → redecode checks. A Python section reassembly alone cannot verify those conversions.
3. Implement MM6 first-visit and timed-reset ordering for visibility, actor, chest, door and quest/map state. Test both indoor and outdoor transitions, including a save made before leaving, after returning, and immediately after a reset.
4. Implement `.ec6` save-policy isolation in every caller and load-menu path. Test wrong title, future version, truncation, count overflow, and failed replacement while retaining the current session and previous good save. Confirm the frozen MM7 baseline still loads independently.
5. At F5, compare the graphical view before and after transition/save/reload; inspect the map, door/chest state, party, save metadata and thumbnail. A log-only or parser-only check is insufficient.

## Unresolved before F3 freeze

- Nonzero reset-header and 56-byte tail semantics; all retained baseline fixed regions are zero, so the current corpus cannot settle them.
- Exact MM6 actor, item, chest and door field meanings and which bytes may change when runtime objects move or die.
- Whether the 200 persistent bytes represent exactly 100 MM6 map variables plus other flags, and how all event variable IDs map to them.
- The two anomalous `.blv` compression trailers and their runtime decoding policy; the deltas did not share that exception.
- First-visit and refill rules, including which visibility, quest, chest and NPC state must survive a timed reset.
- Party/NPC/quest/timer/beacon schema for `.ec6`; native `.mm6` whole-save import/export is outside the current commitment and cannot be claimed.

See also: [MM6 save specification](MM6_SAVE_SPEC.md), [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md), [MM6 event and campaign specification](MM6_EVENT_CAMPAIGN_SPEC.md), and [local status](LOCAL_STATUS.md).
