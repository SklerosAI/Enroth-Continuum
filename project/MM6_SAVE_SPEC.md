# MM6 save and map-delta specification (F2)

Status: **layout evidence and implementation contract, not a completed save implementation**. This document is for the private, Mac-first MM6 build beside the frozen MM7 build. The planned F3 review of the original installed MM6 archives has not happened; this F2 review used only retained, ignored `Games.lod` map extracts and the current source tree.

## Evidence and boundary

- `scripts/mm6_probe_indoors.py` parses 52 retained `.blv` maps and their 52 `.dlv` companions to the exact decoded end. `local-game-data/mm6/research-2026-11-05/indoor-probe.json` reports no layout failures. Two **base maps** (`cd3.blv`, `d08.blv`) have invalid zlib checksums despite a framed, size-checked raw DEFLATE stream; no `.dlv` has that exception.
- `scripts/mm6_probe_outdoors.py` parses 15 retained `.odm` maps and 15 `.ddm` companions to the exact decoded end. `local-game-data/mm6/research-2026-11-05/outdoor-probe.json` reports no layout or checksum failures.
- The record boundaries are cross-checked with the [MM6 indoor delta reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/DeltaIndoorDataMap.java) and [MM6 outdoor delta reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/outdoor/DeltaOutdoorDataMap.java) in MM8LevelEditor. The probe checks boundaries and counts, **not** semantic reconstruction of every field.
- The shipped `.dlv` and `.ddm` entries are initial map-state deltas. They do **not** establish the format of a complete native MM6 `.mm6` save. No native save file was inspected in F2, so native import/export and binary compatibility are unproven.

All private reports and game content stay under ignored `local-game-data/`. Do not add map bytes, decompressed records, GOG archives, or save files to Git.

## Observed MM6 delta wire layout

Sizes below are decoded bytes, before the `Games.lod` entry's compression wrapper. Each counted section starts with a four-byte little-endian count. The stated order and end boundaries are observed on all retained companion deltas; the meanings of all bytes within the records are not yet established.

| Section | Indoor `.dlv` | Outdoor `.ddm` |
| --- | --- | --- |
| Opening | 8 bytes: reset count and last reset day | Same 8 bytes |
| Revealed map | 875 outline bytes | Two 88 × 11-byte visibility arrays, 1,936 bytes total |
| Creature records | Count, then 548 bytes each | Same |
| Ground item records | Count, then 100 bytes each | Same |
| Chest records | Count, then 4,204 bytes each: 4-byte header, 140 × 28-byte item slots, 140 × 2-byte grid cells | Same |
| Doors | 200 × 80-byte fixed records, then variable `int16` arrays. Their computed total length equals `doorsDataSize` in the companion `.blv` header for all 52 maps. | No door section in the observed outdoor layout |
| Persistent map bytes | 200 bytes | 200 bytes |
| Tail | 56 bytes, containing visit-time/sky/weather/reserved fields per probe; field semantics and preservation still need verification | Same 56 bytes |

The MM6 deltas have no MM7-style per-face attribute and decoration-flag sections between visibility and the counted records. The current MM7 snapshot types **do** have such sections. Do not treat the equal 200-byte persistent-variable total as evidence that MM6 uses MM7's 75 `mapVars` + 125 `decorVars` split; that mapping remains unresolved. Preserve all 200 bytes and the 56-byte tail losslessly until their semantics are proved. Preserve unrecognized bytes within individual records as well.

The MM6 8-byte opening cannot be read as `LocationHeader_MM7` (40 bytes). The inherited MM7 record sizes are 836 bytes for `Actor_MM7`, 112 for `SpriteObject_MM7`, 36 for `Item_MM7`, and 5,324 for `Chest_MM7`; the observed MM6 delta records are respectively 548, 100, 28 within chests (100 for ground items), and 4,204 bytes. Both titles have 80-byte fixed indoor door records, but that equal size does not prove identical field semantics. These MM7 sizes come from static assertions in `src/Engine/Snapshots/EntitySnapshots.h`. A title-specific decoder and encoder are required; changing only the filename extension would corrupt map state.

## Save and profile contract

1. MM6 uses its own versioned **Enroth Continuum** save container, `.ec6`, beneath `~/Library/Application Support/Enroth Continuum/Profiles/MM6/saves/` on macOS. `resolveMm6UserPath()` already selects the MM6 profile by default. Manual, autosave, and quicksave names and the load-menu filter must all select `.ec6` for MM6. The extension identifies our format, not native MM6 compatibility.
2. MM7 continues to use its own profile and existing `.mm7` saves and serializers. Its frozen checkout and installed playable build are regression baselines. No MM6 path may scan, migrate, overwrite, or deserialize an MM7 save, even if `--user-path` is pointed at a common parent. The MM6 save reader must reject another title or unsupported schema version from the file header before reconstructing state.
3. Keep installed GOG game data read-only. The user profile holds saves, settings, logs, thumbnails, caches, and mods; MM6's paths must be title-specific. The MM6 launcher must supply its intended profile explicitly if it overrides the default. Profile separation is the immediate safety boundary, with in-file title/version checks as a second boundary.
4. Define a versioned `.ec6` header with an MM6 title identifier, format version, bounded section sizes and an integrity check. Store the active map, party/character state, clock, NPC/quest/event state, overlays, map deltas, beacon images, and thumbnail as title-specific data. A source-install manifest or content fingerprint is useful for diagnostics, but should not block a legitimate reinstall unless a proven incompatibility exists. Exact `.ec6` field schema must be fixed after the MM6 runtime structures and native MM6 party/NPC evidence are audited.
5. On save, write a temporary file in the same profile, validate the completed container, then atomically replace the destination while retaining a recoverable previous save. On load, bound counts and lengths, reject truncation and cross-title data, and leave the current session untouched if validation fails. This is especially important for autosave and quicksave, which routinely replace existing files.

The application can initially store MM6 map deltas as losslessly encoded MM6 wire blobs inside `.ec6`, or use an explicitly versioned internal state representation. Either choice must preserve unknown bytes and pass MM6 decode → reconstruct → modify → encode/reload checks. A native MM6 `.mm6` save importer/exporter is a separate, uncommitted feature that requires native save samples and format validation.

## Current source gaps

| Runtime path | Current behavior | MM6 requirement |
| --- | --- | --- |
| `src/Engine/SaveLoad.cpp` | `loadGame()` always uses `SaveGame_MM7`; map saves always use `IndoorDelta_MM7` / `OutdoorDelta_MM7`; final container always uses `SaveGame_MM7`. New game copies `.dlv` / `.ddm` bytes but later code reads them through MM7 types. | Dispatch **before** reading/writing a save or delta. Implement MM6 container and indoor/outdoor snapshot adapters, including initial map state. |
| `src/Engine/SaveLoad.h`, `src/Engine/SaveLoad.cpp` | `autosave.mm7`, `saveNNN.mm7`, and `quicksaveN.mm7` are hard-coded. | Use `.ec6` names for MM6 through a title-specific save policy. Keep MM7 names unchanged. |
| `src/GUI/UI/UISaveLoad.cpp` | Filters only `.mm7`, then reads `SaveGameLite_MM7`. | MM6 load menu filters `.ec6` and reads a minimal MM6 header/thumbnail; invalid slots are skipped with diagnostics. |
| `src/Engine/Graphics/Indoor.cpp`, `Outdoor.cpp` | Loads map state through `IndoorDelta_MM7` / `OutdoorDelta_MM7`. Outdoor base map already has an MM6-specific decoder, but the delta path does not. | Decode shipped MM6 deltas and saved MM6 deltas through MM6 adapters; verify map transition and reset behavior. |
| `src/Engine/Snapshots/CompositeSnapshots.*`, `EntitySnapshots.h` | `SaveGame_MM7` writes an MM7 LOD (`LOD_VERSION_MM7`, `chapter`, `newmaps for MMVII`), with MM7 party/NPC snapshots and map delta types. `LocationHeader_MM7` is 40 bytes; `PersistentVariables_MM7` splits 200 bytes 75/125. | Create MM6-specific party/character/NPC/quest and delta snapshots where evidence demands them. Never label the existing MM7 LOD as MM6 by renaming the extension. Prove the MM6 persistent-byte mapping before translating it into runtime variables. |
| `src/Application/Startup/PathResolver.cpp`, `GameStarter.cpp` | macOS MM6 profile path exists; save migration logic is MM7-only. | Keep MM6 profile isolated; add only deliberate MM6 migration if an actual old MM6 app format exists. No automatic MM7 migration. |

MM6's event-variable numbering is also title-specific. A 200-byte map-state block does not justify reuse of MM7 event ID arithmetic; reconcile the retained event-variable probe with the MM6 runtime mapping before implementing reads or writes to those bytes.

## Transition and write-path audit

This F2 source review adds consumer obligations, not new field semantics.
`Indoor.cpp` and `Outdoor.cpp` currently use MM7 header counts and
`lastRespawnDay` during initial and timed reset. They preserve selected
visibility state, restore shipped deltas, and rewrite counts. The observed
MM6 eight-byte opening cannot be interpreted through that 40-byte header.
The reset-day meaning, map refill rules and which MM6 state survives a refill
must be reconciled before a title-specific implementation.

`CompositeSnapshots.cpp` reconstructs indoor door pointers and geometry;
`Indoor.cpp` also consumes door state, timer and speed while updating that
geometry. MM6's equal fixed door-record size does not prove equal semantics.
Offline round trips must cover closed, open and mid-motion doors, the variable
arrays bounded by the base-map header, and invalid face references. Chest
round trips must cover the MM6 14 × 10 grid, item identity, opened/trap flags,
loot removal and revisit; the current `ChestTable.cpp` remains an MM7 9 × 9
table.

`SaveLoad.cpp` copies shipped map deltas at new game, snapshots only the active
map later, and copies other map blobs into the save. It updates the in-memory
delta collection before a direct `ufs->write`, with no atomic replacement or
prior-save recovery. An MM6 `.ec6` writer must avoid leaving either disk or
memory in a half-updated state on write failure. Audit manual, autosave and
quicksave callers, plus `Engine.cpp`'s pre-transition autosave and the direct
travel, portal, beacon and house transition sites. `Outdoor.cpp`'s foot-travel
table is MM7-specific; MM6 transition and respawn order must be tested against
MM6 map/event semantics. The existing MM6 profile resolution in
`PathResolver.cpp` is a useful boundary, but it does not make the current
MM7 container or load-menu filter safe for MM6.

Acceptance includes strict decode and field-preserving re-encode of all 67
retained deltas; synthetic mutation of a chest item/flag, ground item, door
state/timer, visibility bit and persistent byte; transition, save/reload and
re-entry; and negative tests for truncation, count overflow, wrong map/title,
unsupported version and failed atomic replacement. Run both initial and
timed-reset cases. Keep native `.mm6` whole-save compatibility as a separate,
unproven feature unless the owner later requests import/export.

## Verification gates for implementation

1. **Pure data:** decode all 52 indoor and 15 outdoor shipped delta companions with strict bounds and exact end; re-encode/redecode each without semantic loss, including reserved bytes. Check base-map anomalies separately and do not weaken delta checksum validation because two `.blv` bases have unusual trailers.
2. **World state:** open a chest, alter loot, move a ground item, operate a door, reveal a map cell, set a map/quest variable, leave the map, return, save, quit, relaunch, load, and verify each persists. Cover indoor and outdoor transitions and map reset/respawn behavior.
3. **Party state:** verify character stats, skills, spells, conditions, inventory/equipment, gold, NPCs, quests, clock, overlays, beacons, and active character across manual, autosave, and quicksave cycles. Match the actual MM6 roster/party model after its data audit rather than presuming MM7's fixed snapshot structures.
4. **Isolation and recovery:** open the MM6 load menu with MM7 `.mm7` files present; ensure they are invisible and unmodified. Exercise corrupt, truncated, wrong-title, and future-version `.ec6` files without losing the current session or previous valid save. Relaunch frozen MM7 and confirm an existing MM7 save still loads.
5. **User-visible:** from the installed Mac app, inspect the rendered map and party after load and after a transition. Verify the save thumbnail and load-menu metadata visually, not only through log output.

## F3 questions before implementation lock

- Inspect an authorized local native MM6 save **only if native import/export becomes a goal**. Until then, avoid claiming `.mm6` compatibility; `.ec6` can be self-contained.
- Resolve the meaning of the 200 map bytes, including any split and MM6 event ID mapping, and the 56-byte tail. Confirm the precise actor, item, chest, and door field mappings needed for runtime reconstruction, not just their sizes.
- Decide how untouched shipped delta blobs and changed deltas are retained across saves; establish a lossless round-trip for every map and define compression/checksum policy.
- Confirm respawn/reset semantics and the handling of the two anomalous `.blv` compression trailers without altering original game data.
- Audit MM6 party, character, spell, NPC, quest, timer, beacon, and thumbnail data against actual MM6 evidence before freezing the `.ec6` payload schema. The inherited MM7 `party.bin` and related entries are not evidence of MM6 layout.
