# MM6 save format

How the MM6 port saves and loads a game: the `.ec6` container, its sections and versions, the write and load paths,
the slot policy, and what a save does and doesn't hold.

The MM6 port has its own save format. It does not read or write original MM6 `.mm6` saves, and it never touches the
MM7 `.mm7` saves or the MM7 profile. An importer for original saves would be a separate feature, no native MM6 save
was ever inspected.

## Container

Every MM6 save is one EC6 container, encoded and decoded by `mm6::encodeSaveContainer` and
`mm6::decodeSaveContainer` in `MM6SaveContainer.cpp`. All integers are little-endian.

File header, 24 bytes:

| Offset | Size | Field |
|---|---|---|
| 0 | 8 | Magic `EC6SAVE\0` |
| 8 | 2 | Container version, 1 to 7 accepted by the container layer |
| 10 | 2 | Flags, must be 0 |
| 12 | 2 | Section count, 1 to 32 |
| 14 | 2 | Reserved, must be 0 |
| 16 | 4 | Total file size, must equal the real size |
| 20 | 4 | CRC-32 (IEEE, reflected `0xEDB88320`) of everything after the header |

Each section follows with a 16-byte header and its payload:

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | Tag, four characters from `A-Z`, `0-9` and `_` |
| 4 | 2 | Flags. Bit 0 is "required", every other bit must be 0 |
| 6 | 2 | Reserved, must be 0 |
| 8 | 4 | Payload length, at most 32 MiB |
| 12 | 4 | CRC-32 of the payload |

Limits: the file is at most 64 MiB, tags are unique, and nothing may follow the last section. The decoder checks the
header, every section header and both checksums before it returns anything. A required section with a tag outside
the container's known-tag list is refused. An optional section with an unknown tag is kept as opaque bytes.

The container layer only frames bytes. Which sections a given version must hold is checked one level up by the
manifest decoders (`decodeRawSave`, `decodeSessionSave`, the v3 to v6 decoders and the slot shells in
`MM6AtomicSaveFile.cpp`). Those decoders want an exact section set. A missing, extra or duplicate section is refused,
and so is a required section marked optional or the other way round. The v7 slot shell also wants `PRKM` after the
19 raw owners, then an optional `TIMR` and an optional `ARTC` last.

### Section payloads

Every payload except `ACTR` starts with its own one-byte payload version, separate from the container version. `ACTR`
starts with the four bytes `ACTR` instead. Decoders accept exactly the version they write. There is no migration
between payload versions, a save with any other version byte is refused. Changing a payload layout means bumping its
version byte and deciding what happens to saves written before, see [Changing a section](#changing-a-section).

## Container versions

| Version | Sections | Status |
|---|---|---|
| 1 | 18: the v2 set with logical `ITEM` and `HELD` v1 in place of `INVR` and raw `HELD`, and no `SHOP` | Not written by the game. `KnownSaveFields` and the v1 atomic writer only serve unit tests. |
| 2 | The 19 raw owners, see below | Never written as a file by the game. It is the core of v3 to v7, and the new game stages its first save as v2 bytes in memory. |
| 3 | v2 + `ACTR`, `GNPC`, `RNGS` (22) | Only nested inside v4 and v5. |
| 4 | v3 + `EVRT` (23) | The first-map anchor kept in memory, see [First-map saves](#first-map-saves-v5). |
| 5 | v4 + `SPRT`, `ARTR`, `DECR` (26) | Written: a save on the first map of a session that never left it. |
| 6 | Outdoor: v5 + `VIST` (27). Indoor: v2 + `GNPC`, `RNGS`, `EVRT`, `IFOV`, `IACR`, `ISCR`, `IDCR`, `IDRT`, `ISRT`, `IART` | Test-only candidates. No production writer or loader. |
| 7 | v2 + `PRKM`, then an optional `TIMR` and an optional `ARTC` (20 to 22) | Written: every other save. `SESSION_SAVE_VERSION`. |

A slot file on disk is always v5 or v7. The slot shell (`decodeNumberedSlotShell`) picks v7 when the container version
is 7 and v5 otherwise, and both validators run the full staging pass before a file is trusted.

## Section table

The 19 raw owners, in file order. `RawSaveFields` holds their decoded form.

| Tag | Payload ver. | Codec | What it stores |
|---|---|---|---|
| `PART` | 6 | `MM6CoreSaveFields.cpp` | Party scalars, see below |
| `CHAR` | 4 | `MM6CoreSaveFields.cpp` | Four characters, see below |
| `QEST` | 1 | `MM6DurableSaveFields.cpp` | Quest bits 1 to 512, autonote bits 1 to 208, award bits and character event bits per character |
| `EFFT` | 1 | `MM6DurableSaveFields.cpp` | The 19 condition clocks of each character |
| `INVR` | 1 | `MM6RawInventoryFields.cpp` | Each character's 138 native 28-byte MM6 item records, 126 grid index words and 16 equipment index words, kept bit for bit |
| `HEAD` | 1 | `MM6SaveMetadata.cpp` | Display name, active map file name, playing time |
| `TIME` | 1 | `MM6SaveMetadata.cpp` | Simulation clock ticks. The frame delta and OS tick are not saved |
| `UIST` | 1 | `MM6SaveMetadata.cpp` | Active character (must agree with `PART`) and a reset-transient-UI flag |
| `VISU` | 1 | `MM6SaveMetadata.cpp` | The 50 active screen overlays. A v7 save writes an empty list, hit flashes over actors don't survive a load |
| `MAPS` | 1 | `MM6MapSaveFields.cpp` | The native `.ddm`/`.dlv` delta of all 67 maps of the title, as archive entries, see below |
| `RESC` | 1 | `MM6NpcSaveFields.cpp` | All 501 NPC records, 51 NPC group words and the active count, as a port-defined record, not a native layout |
| `RSTS` | 1 | `MM6ResistanceSaveFields.cpp` | Base and bonus values of the 11 resistance channels per character |
| `HELD` | 2 | `MM6RawHeldItemSaveFields.cpp` | The cursor item as one native 28-byte record. A save is refused while an item is on the cursor, so it is always empty in practice |
| `BUFF` | 1 | `MM6BuffSaveFields.cpp` | 20 party and 4 × 24 character buff slots: expiry, power, mastery, overlay, caster, GM flag |
| `HIRE` | 1 | `MM6HirelingSaveFields.cpp` | The two hireling NPC copies, their display names and their sacrifice timers |
| `RCVR` | 3 | `MM6RecoverySaveFields.cpp` | Recovery clocks, daily spell counters, event clocks and the Armageddon state, see below |
| `CHST` | 1 | `MM6ChestSaveFields.cpp` | The chest-generation LCG state, the 30 generated-artifact flags, and the active map's decoded delta as the source for later chest writes |
| `SCLK` | 1 | `MM6ShopGuildClockFields.cpp` | 68 refresh deadlines: shop rows 1 to 47 and guild rows 119 to 140. Shop 47 and guild 119 share a slot, as they do in the original |
| `SHOP` | 1 | `MM6ShopStockFields.cpp` | 120 × 12 native 28-byte shop and guild stock records, the physical layout the original new-game initializer fills |

Details for the rows above:

- `PART`: gold, food, bank gold, playing time, position, yaw and pitch, active character, vertical velocity, flight Z,
  water and lava timer, fall start Z, flying, the two party flag words, fine, deaths, prison terms, bounties
  collected, reputation, bribes, the tavern drink bits and the 21 tavern tips. Then the bounty of each of the three
  town halls (New Sorpigal, Misty Islands, Silver Cove), each as the time of its next monster roll, the monster and
  its killed flag. The capture refuses bounty state in the inherited fourth and fifth town hall slots.
- `CHAR`: name, sex, class, face, voice, level, experience, HP, SP, skill points, 7 base stats, 31 skills, 99
  learned-spell flags, stat bonuses, AC, level and age modifiers, birth year, the 7 pure-stat-potion flags, quick
  spell, spellbook page, and 8 raw runtime bonus bytes whose original meaning is unknown.
- `MAPS`: the active map's entry is the delta it would be parked with now. A visited map's entry is the delta it was
  parked with. A map the party never entered keeps its shipped delta.
- `RCVR`: last regeneration, days without rest, recovery and turn-based recovery per character, the daily Divine
  Intervention, Armageddon and Fire Spike counters, the 59 event clocks (10 counters, 29 history, 20 others), and the
  Armageddon timer, damage and push count. Its playing time must agree with `PART` and `HEAD`.

Added by v3 to v7:

| Tag | Payload ver. | Codec | In | What it stores |
|---|---|---|---|---|
| `ACTR` | magic | `MM6ActiveActorSection.cpp` | v3+ | The active map's whole actor list in PID order |
| `GNPC` | 1 | `MM6GeneratedNpcSaveFields.cpp` | v3+, v6 indoor | Generated NPC rows of the active map, joined to their actor slots |
| `RNGS` | 1 | `MM6GameplayRngSaveFields.cpp` | v3+, v6 indoor | Gameplay random generator state |
| `EVRT` | 2 | `MM6EventRuntimeSaveFields.cpp` | v4+, v6 indoor | Per visited map: respawn count and day, map and decoration variables, decoration bindings |
| `SPRT` / `ISRT` | 1 | `MM6V5SpriteRuntimeFields.cpp` | v5+ / v6 indoor | Runtime fields of ground sprites that the native record narrows |
| `ARTR` / `IART` | 1 | `MM6V5ActorRuntimeFields.cpp` | v5+ / v6 indoor | Runtime fields of actors that the native record narrows |
| `DECR` | 1 | `MM6V5DecorationRuntimeFields.cpp` | v5+ | Outdoor decoration runtime state |
| `VIST` | 2 | `MM6VisitedMapSection.cpp` | v6 outdoor | Previously visited outdoor maps |
| `IFOV` | 1 | `MM6IndoorFaceOverrides.cpp` | v6 indoor, optional | Indoor face attribute and texture changes against the map's pre-event base |
| `IACR`, `ISCR`, `IDCR` | 1 | `MM6V6IndoorSaveFields.cpp` | v6 indoor | Native actor, sprite and decoration slot anchors of the active `.blv` |
| `IDRT` | 1 | `MM6V6IndoorSaveFields.cpp` | v6 indoor | Indoor decoration runtime state |
| `PRKM` | 1 | `MM6ParkedMapSection.cpp` | v7, required | The live lists of the active map and every visited map, see below |
| `TIMR` | 1 | `MM6MapTimerSection.cpp` | v7, optional | Per map, when each of its four periodic event timers last fired |
| `ARTC` | 1 | `MM6ArtifactCollectionSection.cpp` | v7, optional | The All Artifacts mod's 30 collected flags of artifacts 400 to 429 |

`PRKM` holds, per map of the session and sorted by name: the map name, the 75 map and 125 decoration event
variables, the actor list as full actor images, and the ground-item sprite list.

`BEAC` (Lloyd's Beacons and a thumbnail, `MM6BeaconSaveFields.cpp`) and `SPAW` (`MM6SpawnedActorSection.cpp`) have
codecs and unit tests but are in no manifest. `ITEM` and v1 `HELD` belong to v1 only.

`PRKM` must round-trip exactly. The encoder re-decodes its own output and refuses a payload that doesn't come back
byte for byte, and the decoder refuses a payload that doesn't re-encode to the same bytes. Map names in `PRKM` and
`TIMR` are canonical lower-case `.odm`/`.blv` names.

`TIMR` arrived after v7 shipped, without a container version bump. It is written optional, after `PRKM`, so a v7 save
without it still loads, and its maps then start with no recorded firings. It only matters with
`gameplay.fix_map_timers`, see [Travel and maps](mm6-travel-and-maps.md).

`ARTC` came later the same way, after `TIMR`. It is written only once the All Artifacts mod started the ledger, so a
game that never ran the mod saves the same bytes as before. A save without it loads with an empty ledger. A first-map
v5 save has no `ARTC`, see [Mods and the Atlas](mods-and-atlas.md#all-artifacts-for-mm6).

### Native map deltas

`MAPS` and `CHST` carry original-format MM6 map deltas, not a port format. A decoded delta is: an 8-byte reset
header (respawn count and last respawn day), the revealed-map bits, counted actor records of 548 bytes, counted ground
item records of 100 bytes, counted chest records of 4,204 bytes (header, 140 item records of 28 bytes, 140 grid
cells), indoors the door block, then 200 persistent event bytes and a 56-byte visit block (last visit time, sky name,
fog). MM6 deltas have no per-face attribute or decoration flag section, unlike MM7. A chest item record can carry the
GrayFace chest artifact condition `0x400`, see
[Travel and maps](mm6-travel-and-maps.md#artifacts-in-unopened-chests). The delta codecs live in `MM6OutdoorWire.cpp`
and the indoor world plan. [Travel and maps](mm6-travel-and-maps.md) owns the details.

## What is and isn't saved

A v7 save holds the 19 raw owners, every map's delta and every parked map's live lists. A load rebuilds the active map
from its parked delta and lists, the same way the party returning to a map in the same session does. So anything the
return path restores, the load restores.

Not held by a v7 save:

- Lloyd's Beacons. There is no `BEAC` in the manifest, and `requireRawV2NoBeacons` refuses the save while any
  character has a beacon set. The player can't cast Lloyd's Beacon yet, so this doesn't come up in play.
- Carried items without their native record. `INVR` keeps each item's 28-byte record, and the capture
  (`captureBoundedInventory` in `MM6BoundedInventoryProjection.h`) writes it back for items without a bonus, gold,
  artifacts and relics 400 to 429 (`MM6PlainItemProjection.h`), items with a standard bonus
  (`MM6StandardItemProjection.h`) and items with any of the 58 special bonuses the generator gives
  (`MM6SpecialItemProjection.h`). A capture writes back only the fields a projection maps, the identified and broken
  bits, a wand's charges, a gold pile's amount and a standard bonus. Every other byte, the chest artifact condition
  `0x400` among them, goes back as it came. An item that came without a record, or whose record the projections
  don't accept, makes the capture throw and the save is refused. Items on the ground and in chests are not limited
  this way, they ride in `PRKM` and the map deltas.
- Generated NPCs and the gameplay random state. v7 has no `GNPC` or `RNGS`. Street NPCs come back the way they do on
  a map return.
- Indoor face changes. v7 has no `IFOV`, so a face change survives only when the map's reload script reapplies it.
- A thumbnail. The load list shows the display name, map and playing time only.
- Transient state: the cursor item (saving with one is refused), screen overlays, an in-flight map change (refused),
  active rest or turn-based mode (refused by the `RCVR` capture), frame timing.

A first-map v5 save is the exception where the actor, sprite and decoration runtime sections hold the active map
directly instead of `PRKM`.

## Saving

Entry points, all in `src/GUI/UI/MM6SaveLoadMenu.cpp` and reached from `src/Engine/SaveLoad.cpp` when the title is MM6:

| Path | Function | Slot |
|---|---|---|
| Save menu | `mm6SaveFromMenu`, then `saveFirstMapManualSlot` | The selected `saveNNN.ec6`, or the first free one |
| Autosave | `mm6AutoSave`, then `saveSessionSlot` | `autosave.ec6` |
| Quick save, F5 | `mm6QuickSave`, then `saveSessionSlot` | `quicksaveN.ec6`, rotating with `gameplay.QuickSavesCount` |

`mm6::saveSlot` in `MM6FirstMapManualSave.cpp` does the work. It needs an armed session (see below) and a profile root,
and refuses an item on the cursor. Then it either captures a first-map v5 save or calls `captureSessionSave` for v7,
and commits the bytes through `writeNumberedSlotCandidate`. It never throws. It returns one of:

| Outcome | Meaning | Menu message |
|---|---|---|
| `MANUAL_SAVE_COMMITTED` | New primary and backup are durably in place | `LSTR_GAME_SAVED` on the status bar |
| `MANUAL_SAVE_REFUSED` | Nothing written: unarmed session, no root, or state the save can't hold | "This game can't be saved here yet" |
| `MANUAL_SAVE_NOT_WRITTEN` | The write failed before the primary was replaced, the slot is unchanged | "Save failed, the slot was not changed" |
| `MANUAL_SAVE_UNCONFIRMED` | The write failed after the primary may have been replaced | "Save could not be confirmed..." |

A failed quick save rolls the quick-save index back. An autosave's outcome is only logged, gameplay goes on.

`captureSessionSave` is read-only. It refuses when no map is settled, a map change is pending, an item is on the
cursor, a beacon is set, or the map cache shape changed. It takes the raw owners from the session template (the owners
the session was created or loaded with), overwrites every owner it can capture live, captures every map with
`captureSessionMaps` (the active map parked as leaving it now would park it), fills `MAPS` from the parked deltas and
`pMapDeltas`, and builds `CHST` from the active delta. The inventory capture keeps the template's native item records
for items that didn't change.

### Arming

A session can be saved only once it is armed (`isManualSaveArmed`). A new game arms itself when the first map is ready
(`armNewGameSessionAtFirstMap`), and a load arms the loaded session. A new game whose first save couldn't be staged
stays unarmed and shows "This party can't be saved yet". Starting a new game or loading disarms the old session.

### First-map saves (v5)

A new-game session starts with a v4 anchor in memory (`armFirstMapManualSave`): the staged new-game owners plus the
native actor slots, `ACTR` and the `EVRT` decoration bindings of the first map, `oute3.odm`. While the party has never
left that map, a save is a v5 save built from the anchor (`writeV5FirstMapNativeLiveSave`). The first `leaveMap`
drops the anchor and every later save of the session is v7. Loading a v5 save restores the anchor, so a session loaded
on the first map saves v5 again until it leaves.

### Autosaves

The port autosaves through `autoSave()`:

- After a new game, once the party stands on the first map (`Game.cpp`).
- On a change to a different map through `startMapTransition`: map-change events, dungeon doors, house exits and
  coach and boat trips. The active map is parked first, so the autosave sees it as left.
- On Town Portal and Lloyd's Beacon, whose books call `autoSave()` themselves. The capture parks the active map the
  way leaving it would. Neither spell is castable by the MM6 player yet.
- On a confirmed walk across an outdoor map border, before the trip's days, rest and food apply, with the party still
  at the edge (`confirmBorderTravel`).
- With `gameplay.fix_travel_autosave`, a coach or boat trip pays its fare and spends its days after the autosave, so
  loading the autosave doesn't lose the fare.

No autosave is written while the Hive finale is underway, or in a session that isn't armed.

## Loading

| Path | Function | Notes |
|---|---|---|
| Main-menu Load | `loadGame`, then `mm6::loadFirstMapManualSlot` | Throws on a bad slot, the caller abandons the load |
| Escape-menu Load, in-game quick load | `mm6LoadOverGame`, then `mm6::loadSlotOverSession` | Stages first. A slot that can't be staged leaves the running game as it was |

Main-menu quick load is refused for MM6.

A load runs in two phases. Staging reads the primary, decodes it and builds every owner against the loaded tables and
archives without touching the running game: `stageSessionSave` for v7, `V5FirstMapLoadAttempt::fromNativeSlot` for v5.
Staging checks that every map is in the title's map table, that every item is an MM6 item template, that the
active map has `PRKM` lists, and that the raw owners agree with each other (clocks, active character, active map,
chest delta). Publication then replaces the party, NPCs, timer, overlays, map deltas, chest and shop owners and the
parked maps, and requests the active map as a savegame load. A load over a running game drops its parked maps and
anchor only after staging succeeded. A throw during publication leaves the session partial and is reported as an
error.

## Slots and atomic writes

Saves live in `<profile>/saves/`. The profile root is the MM6 user path resolved at startup
(`setStartupNativeProfileRoot`), including an explicit `--user-path`. By default it is
`~/Library/Application Support/Enroth Continuum/Profiles/MM6` on macOS and `~/.enroth-continuum/mm6` on Linux. A RAM
user filesystem gives no root and no saves.

`isMM6SaveSlot` admits three name forms: `saveNNN.ec6`, `autosave.ec6` and `quicksaveN.ec6`. Each slot is a primary
file and a `<slot>.bak` backup.

Write rules (`MM6V5FirstMapNativeSlotPolicy.cpp` and `writeAtomicSaveFileForSchema` in `MM6AtomicSaveFile.cpp`):

1. The candidate passes the v5/v7 slot shell and the full staging validator.
2. Every existing primary and backup of the slot must also be a valid v5 or v7 save. A damaged file, another
   container version, a backup without a primary, a non-regular file or a symlinked `saves` folder refuses the
   write before any file changes.
3. The candidate goes to a `mkstemp` temp file `.<slot>.tmp.XXXXXX` in the same folder, is fsynced, closed and read
   back. The readback must match byte for byte.
4. When a primary exists, its bytes go through a second temp file that is renamed over `<slot>.bak`, then the folder
   is fsynced.
5. The temp file is renamed over the primary and the folder is fsynced again.

A crash leaves either the old primary, or the new primary with the old one as backup. The fault seam
`detail::AtomicSaveStep` lets unit tests stop at each step. The writer is POSIX only and throws on Windows.

Read rules: listing and loading never repair disk. The load list (`listV5FirstMapNativeSlots`) shows only slots whose
primary passes the full validator. The menu sorts them by display name. Backup-only, damaged and foreign slots are
left out. A slot that would need backup recovery is refused for load.

## Changing a section

- A new field in an existing section: bump that section's payload version byte, update encoder, decoder and capture,
  and decide what happens to saves with the old byte. Today they are refused, nothing migrates.
- A new section that old saves can live without: add it optional and after the existing ones, the way `TIMR` was
  added. Teach `decodeV7SlotShell` and `decodeSessionSave` to accept it present or absent. Optional sections need no
  entry in the container's known-tag list.
- A new required section: add its tag to `KNOWN_TAGS` in `MM6SaveContainer.cpp`, raise `MAX_VERSION`, bump
  `SESSION_SAVE_VERSION` and the slot shell, and accept or refuse the old version on purpose.
- Keep the decoder strict: exact sizes, bounded counts, canonical names, and every check before any owner is
  assigned.

## Source

- [MM6SaveContainer.cpp](../src/Engine/MM6/MM6SaveContainer.cpp) container framing and known tags
- [MM6SessionSave.h](../src/Engine/MM6/MM6SessionSave.h) v7 encode, decode, stage, capture, publish
- [MM6RawSaveFields.h](../src/Engine/MM6/MM6RawSaveFields.h) the 19 raw owners
- [MM6V5SaveFields.h](../src/Engine/MM6/MM6V5SaveFields.h), [MM6V4SaveFields.h](../src/Engine/MM6/MM6V4SaveFields.h), [MM6V3SaveFields.h](../src/Engine/MM6/MM6V3SaveFields.h) first-map save layers
- [MM6ParkedMapSection.h](../src/Engine/MM6/MM6ParkedMapSection.h), [MM6MapTimerSection.h](../src/Engine/MM6/MM6MapTimerSection.h),
  [MM6ArtifactCollectionSection.h](../src/Engine/MM6/MM6ArtifactCollectionSection.h) v7 sections
- [MM6FirstMapManualSave.h](../src/Engine/MM6/MM6FirstMapManualSave.h) arming, save and load entry points
- [MM6V5FirstMapNativeSlotPolicy.h](../src/Engine/MM6/MM6V5FirstMapNativeSlotPolicy.h) slot names and slot policy
- [MM6AtomicSaveFile.h](../src/Engine/MM6/MM6AtomicSaveFile.h) atomic writer and slot shells
- [MM6MapTransition.h](../src/Engine/MM6/MM6MapTransition.h) map parking used by save and load
- [MM6RawV2Admission.h](../src/Engine/MM6/MM6RawV2Admission.h) beacon and face refusals
- [MM6SaveLoadMenu.cpp](../src/GUI/UI/MM6SaveLoadMenu.cpp) menu, autosave and quick save
- Unit tests: [MM6SaveContainer_ut.cpp](../src/Engine/MM6/Tests/MM6SaveContainer_ut.cpp),
  [MM6AtomicSaveFile_ut.cpp](../src/Engine/MM6/Tests/MM6AtomicSaveFile_ut.cpp),
  [MM6ParkedMapSection_ut.cpp](../src/Engine/MM6/Tests/MM6ParkedMapSection_ut.cpp),
  [MM6MapTimerSection_ut.cpp](../src/Engine/MM6/Tests/MM6MapTimerSection_ut.cpp),
  [MM6RawSaveFields_ut.cpp](../src/Engine/MM6/Tests/MM6RawSaveFields_ut.cpp) and one `_ut.cpp` per section codec
- Game tests: [GameTests_MM6Autosave.cpp](../test/Bin/GameTest/GameTests_MM6Autosave.cpp),
  [GameTests_MM6SaveAnywhere.cpp](../test/Bin/GameTest/GameTests_MM6SaveAnywhere.cpp),
  [GameTests_MM6Save.cpp](../test/Bin/GameTest/GameTests_MM6Save.cpp). The `DISABLED_` fresh-process pairs run
  through [run_mm6_firstmap_gate.py](../tools/run_mm6_firstmap_gate.py)

See also: [MM6 travel and maps](mm6-travel-and-maps.md), [MM6 port architecture](mm6-port-architecture.md),
[Testing](testing.md), [Source and privacy](source-and-privacy.md), [Open questions](open-questions.md)
