# MM6 travel and maps

How the port loads MM6 outdoor and indoor maps, keeps their state across visits, and moves the party between them,
with the MM6 map formats where they differ from MM7.

## Map list and naming

MM6 has 67 maps in `mapstats.txt`: 15 outdoor maps and 52 indoor ones.

- Outdoor maps are named `outXY.odm` with column `X` in `a` to `e` and row `Y` in `1` to `3`. New Sorpigal, the start
  map, is `oute3`. The base map is `.odm`, the per-save state is `.ddm`.
- Indoor maps are `.blv` with state in `.dlv`. Names are short stems such as `dNN` dungeons, `tN` temples, `cdN`
  castles, and named maps like `hive`, `pyramid` and `sewer`.
- Map entries in the MM6 LOD archives have an 8-byte header (compressed size, decoded size) followed by zlib data
  (`decodeMM6ArchiveEntry` in `MM6Archive.h`). MM7 uses a 16-byte wrapper. Two shipped base maps, `cd3.blv` and
  `d08.blv`, have a bad zlib checksum trailer, and only those two get a raw-DEFLATE fallback with full length checks.
- The tavern news region and the transport routes number maps by their 1-based position among the base maps at the
  front of the Games LOD directory, not by `mapstats.txt` row.

`tools/mm6_map_name_manifest.py` builds and verifies a private manifest of `.ddm` and `.dlv` names for the EC6 `MAPS`
section from local research reports. It prints only aggregates and writes under the ignored `local-game-data/`.

`tools/run_mm6_map_sweep.py` runs `MM6MapSweep.DISABLED_EnterTickAndFireEvents` on every map, one headless process per
map, and reruns a map without actors, without the timer wait, or past a failing event to sweep the rest. Results go to
an ignored folder under `local-game-data/`.

## Outdoor format

`decodeOutdoorBase` and `decodeOutdoorDelta` (`MM6OutdoorWire.h`) parse the decoded payloads with exact-end checks and
keep every byte, including fields with no known meaning, so the port can write them back unchanged.

`.odm` base:

| Part | Layout |
|---|---|
| Header | 160 bytes, then four 4-byte tile selectors, then three 128 x 128 byte maps (height, tile, attributes) |
| Models | Count, 188-byte headers, then per model its vertices (12 bytes), 308-byte faces, 2-byte face order, 10-byte texture names and BSP nodes |
| Decorations | Count, 28-byte records, then 32-byte names |
| Decoration PIDs | Count, 2-byte ids, then a 128 x 128 x 4 byte map |
| Spawn points | Count, 20-byte records |

`.ddm` delta, in order:

| Part | Size |
|---|---|
| Reset header | 8 bytes: respawn count and last respawn day |
| Visibility | Two 88 x 11 byte arrays |
| Actors | Count, then 548-byte records |
| Ground items | Count, then 100-byte records |
| Chests | Count, then 4,204-byte records |
| Persistent bytes | 200 bytes. The first 100 are the event map variables. |
| Visit block | 56 bytes: last visit time, the sky shown on that visit, fog fields |

MM6 lacks the 32-byte header extension MM7 puts after the reset header. Actor, ground item and chest records are
smaller than MM7's (548 against 836, 100 against 112, 28-byte items against 36). Unknown bytes ride along in
`mm6DeltaBytes` on `OutdoorLocation` and `IndoorLocation`.

### Terrain

- Each `.odm` has four tile selectors. MM6.exe adds a cell's local tile id to the selected record's signed 16-bit
  base to get the global tile id. The port uses those bases directly. Converting them through MM7's tileset enum
  collapsed MM6 road and terrain types into dirt and gave `pending` textures.
- `TileData` keeps the raw MM6 tileset and variant alongside the shared classification. The raw pair is not a unique
  art key, so the global tile id stays the identity.
- Seasons: the shared season pass changes only grass. For MM6, every tile whose normalized tileset is unchanged keeps
  its authored id, also after the transition pass. MM6's own seasonal display policy has not been traced.
- MM6 land and road bitmaps are 128 x 128, as is the water. The OpenGL terrain path reserves texture unit 1 for MM6
  land so it doesn't share the water unit.
- Walk sounds and the food cost of resting come from the MM6 tile's raw tileset (`walkSoundForTile`,
  `foodRequiredForTile`).

### Sky, sun and fog

- `mm6::outdoorSky` (`MM6OutdoorSky.h`) picks the sky the way MM6's loader does. The first visit shows `sky01`, a
  revisit on the same day of the month shows the stored sky, any other day a random `skyNN`. The `.odm` sky name plays
  no part.
- The sun follows the shared formula from 5:00 to 21:00. Vanilla MM6.exe could keep the light of an earlier daytime
  hour for the first minute after 5:00. `gameplay.fix_sunrise_light` (default on) stores the minute by night too, as
  GrayFace does.
- MM6 fog is not implemented. `SetFog` returns early for MM6, and the visit block's fog fields are carried unchanged.

## Indoor format

`MM6IndoorSnapshot` (`src/Engine/Snapshots/MM6IndoorSnapshot.h`) reads `.blv` and `.dlv`:

- MM6's 80-byte face is MM7's face without its leading 16-byte float plane. The plane is derived from MM6's
  fixed-point one. Faces carry six variable short arrays of `vertex count + 1` entries.
- Rooms keep the 116-byte grouping. A zero first BSP index with no nodes becomes the renderer's -1.
- Decorations are a 28-byte record and a 32-byte name. Lights are 12 bytes and become white lights with MM6 radius and
  brightness. Spawn points are 20 bytes.
- The `.dlv` holds the same 548-byte actors, 100-byte ground items and 4,204-byte chests as outdoors, 200 fixed 80-byte
  door records plus their variable arrays, and 875 visibility bytes. The 200 persistent bytes and the 56-byte visit
  block follow, as outdoors.
- Empty decoration slots in a room's render list are skipped.

## Respawn and refill

On load, a map whose last respawn day is 0 starts from the base delta in the LOD. A map whose refill interval
(`mapstats.txt` days) has passed since its last respawn also starts from the base delta, but keeps its respawn
counters and its revealed map cells. Loading a save never triggers a timed respawn. Otherwise the parked or saved delta
loads as it is.

## Leaving and returning

Every map change runs `mm6::leaveMap` (`MM6MapTransition.h`) before the target loads. It parks the map the party leaves
for the session:

- its delta with the live chests, doors, visibility and respawn counters,
- the visit block stamped with now and the sky shown (`recordMapVisit` in `MM6MapVisit.h`, as MM6's save routine stamps
  it),
- the actors and ground items, which encounters and drops grow past the delta's own records, and the event variables.

On return the parked delta loads instead of the LOD's, and `restoreParkedMap` puts back the lists, unless the map just
respawned. A map whose state can't be parked logs a warning and comes back as last saved.

A save holds every parked map plus the active one (`captureSessionMaps`). The runtime lists go into the required EC6
`PRKM` section (`MM6ParkedMapSection.h`). A loaded save replaces the parked set, so its active map loads the way a
same-session return would. New Game and Load forget all parked maps and timer firings (`forgetParkedMaps`). See
[MM6 save format](mm6-save-format.md).

`MM6VisitedMapCache` and `MM6VisitedMapSection` are an earlier detached design that only unit tests use. The live path
is the parked-map set above.

Party death saves the death map before the week passes, so its last visit is the time of death. See
[combat and monsters](mm6-combat-and-monsters.md).

## Outdoor borders

Walking off an outdoor map's edge offers the neighbouring map (`MM6OutdoorBorderTravel.h`):

- The edge is at plus or minus 22528 on each axis. The neighbour moves the column letter or the row digit, both at a
  corner, and each crossed axis wraps by one map width. There are no special edges. Off the 5 x 3 grid, nothing is
  offered.
- MM6.exe offers only when the party isn't in water damage, airborne or standing on water
  (`borderOfferPartyFlagsPermit`). Its confirmation separately refuses a flying party.
- In vanilla, flying low over an edge cleared those flags and offered a trip that then did nothing. With
  `gameplay.fix_flying_border_offer` (default on) the offer also checks flying, as GrayFace does.
- Travel takes 5 days, less 1 for a Guide, 2 for a Tracker, 3 for a Pathfinder and 1 for an Explorer, at least 1.

Confirming (`confirmBorderTravel`) parks the map, autosaves with the party still at the edge, then applies the trip on
a copy of the party (`MM6OutdoorBorderConfirmation.h`). The trip rests the party fully, rests it once more if it has
any food, eats one food per day, and leaves it Weak if the food ran short. Characters who just became Weak say so. The
party arrives at the wrapped position, and its height is set from the target terrain once the map has loaded.

## Other ways between maps

| Route | Where |
|---|---|
| Event `MoveToMap` (doors, teleporters, dungeon exits) | [MM6 events](mm6-events.md). An all-zero position means Party Start on another map, or stay put on this one. |
| Indoor entry and exit dialog | `MM6TransitionDialog`. Exit pictures 1 to 8 index MM6's own icon table, 0 is no icon. Text comes from `trans.txt`. |
| House exits | `MM6HouseExit.h`, see [houses and services](mm6-houses-and-services.md) |
| Stables and boats | `MM6Transport`, see [houses and services](mm6-houses-and-services.md) |
| Party death | Respawn on `oute3`, see [combat and monsters](mm6-combat-and-monsters.md) |

Town Portal and Lloyd's Beacon are not admitted as MM6 player spells. Beacon save fields exist
(`MM6BeaconSaveFields.h`) for when they are.

## Map timers

Map scripts' `OnTimer` and `OnLongTimer` triggers get their first alarm from the map's last visit in the visit block.
In MM6.exe only an `OnLongTimer` without a half-minute interval can fire at once on entry, when the map was never left
or was left a day or more ago. Every other timer gets a calendar alarm from now. A daily timer's hour byte is used as
is, so an hour past 23 means that many hours after the next midnight.

`gameplay.fix_map_timers` (default on) applies GrayFace's timer fix. Rules and the `TIMR` save section are on
[MM6 events](mm6-events.md#triggers-and-timers).

## Doors and indoor faces

- Doors use the shared door code. The 80-byte MM6 record has the MM7 width. Door action 3, a GrayFace addition that
  switches a moving door, is ignored as in vanilla.
- Pressure plates: MM6.exe runs the event of a plate the party steps on, and also of a plate face the party collides
  with that isn't its floor, every frame it pushes against it. MM6 maps have wall and ceiling plates that rely on this
  (doors, teleports, traps, a password question). The port runs the wall plate event at the end of the frame
  (`Indoor.cpp`). MM7 still ignores wall plates.
- Event face changes (texture and attribute bits) address faces by global index, see [MM6 events](mm6-events.md).
- `MM6IndoorFaceOverrides.h` has a codec for an optional EC6 `IFOV` section that stores changed faces as overrides
  against the base `.blv`, meant to replay after `OnMapReload`. Only the v6 save field writers and the unused
  visited-map section produce it. No live load path applies it.

## Decorations

MM6.exe treats 14 decoration rows as interactive (`isInteractiveDecoration` in `MM6Decorations.h`). With no map
event, clicking one runs global event `decorVars[eventVarId] + 400` and hovering names it by that event's topic. On
respawn, `decorationGlobalEvent` picks the first event with MM6.exe's own odds. `ChangeEvent` (opcode 42) rebinds it.

## Chests

Shipped maps have 20 chest records each, 4,204 bytes: a 4-byte header (id, flags), 140 item slots of 28 bytes, then
140 signed 16-bit grid cells (`MM6ChestPlacement.h`, `MM6ChestGridWire.h`). MM6's map writer saves the whole in-memory
record, grid included.

### Generation

Random chest items are generated for all 20 chests when a map loads (`tryActiveChestMapLoad` in
`MM6ChestMapLoadRoute.h`, called from `Engine.cpp`). The live route is admitted for the first map of a New Game and for
the target of a map change. Any other load keeps the blocking guard, and the MM7 generator throws for MM6.

- Shipped chests hold fixed item ids and random markers -1 to -6. MM6 has a seventh row that always gives tier 7.
- The generator draws once before scanning. Each marker rolls 1 to 5 attempts and remaps its tier with MM6's own 7 x 7
  table against the map's treasure level. The first attempt reuses the previous probability roll, only extra attempts
  draw a new one. Below 20% gives nothing, 20% to under 60% gold, 60% and up an item.
- The item picker draws an artifact candidate from MM6's 30 artifacts on every call. At tier 6 a 5% roll gives it, if
  not yet generated and under the cap of 13. Otherwise it draws against the weights of regular items 1 to 399. A zero
  remainder returns item 1, which is what MM6.exe does.
- Draws use the MM6 gameplay LCG. MM6's initial seed is not known, so the port seeds it once from platform entropy at
  New Game and saves the LCG state in `CHST`.

### Placement

The chest grid is 9 x 9 with fixed pixel offsets, as MM6's chest UI hard-codes. `dchest.bin` carries unused 14 x 10
metadata. Items are placed once in shuffled order, with a footprint of `(pixels - 14) / 32 + 1` cells per axis from the
icon size. In MM6.exe an item that doesn't fit keeps its slot with no cell and is never shown. With
`gameplay.fix_chest_overflow` (W13) such a chest is laid out again, artifacts and quest items first, then the most
valuable, packed into rows when that shows more of them, and reopening it places what had no room.

### Traps

A chest is trapped when flag bits 0 and 1 are both set (`MM6ChestTrapDecision.h`).

- Disarm score (`chestDisarmScore`): Disarm Trap level, doubled for each of artifact 410 worn as a cloak, artifact 415
  in either hand and any unbroken item of Thievery, plus 4 for a Tinker, 6 for a Locksmith and 8 for a Burglar, times
  2, 3 or 4 by mastery.
- With a score above 0, the trap is disarmed when `score + random(10) > 5 * map difficulty`. Otherwise a random fire,
  electricity, cold or poison trap goes off. MM6's cold and electric trap sprites (812, 813) are swapped against their
  MM7 names.
- When the trap sprite expires within 1024 units of the party, each character takes `5 + Nd20` of that element, with N
  from the map's trap dice. A character with Perception avoids it when `random(perception + 20) > 20`, using the raw
  skill byte with its mastery bits.

### Artifacts in unopened chests

An artifact a chest's generation puts on the found list carries condition bit `0x400` in its item record until the
chest's first opening places it in the grid. With `gameplay.fix_unopened_chest_artifacts` (W12, default on), a timed
respawn takes the marked artifacts of the map's old chests off the found list before the chests generate again, so
they can be rolled again. Event opcode 69 does the same for one chest.

## GrayFace travel and map fixes

| Fix | Flag or place |
|---|---|
| Map timers count from their last firing (W1) | `gameplay.fix_map_timers` |
| Sunrise light at 5:00 (W5) | `gameplay.fix_sunrise_light` |
| Coach and boat autosave before the fare (W7) | `gameplay.fix_travel_autosave` |
| Travel speech delay (W8) | `gameplay.fix_travel_speech_delay` |
| No border offer while flying (W9) | `gameplay.fix_flying_border_offer` |
| Ground items don't leak between maps (W11) | By design, each map rebuilds from its own data |
| Artifacts in unopened chests can be found again after a respawn (W12) | `gameplay.fix_unopened_chest_artifacts` |
| Chest items that had no room show later (W13) | `gameplay.fix_chest_overflow` |
| Sprite and texture animation frame timing (W24) | Shared code |

Open: GrayFace's map data fixes (W29, the Free Haven Smuggler's Guild door, is in GrayFace's patched `Outc2.odm`).
The full list is on the [GrayFace patch reference](mm6-grayface-patch-reference.md).

## Source

- [MM6MapTransition.h](../src/Engine/MM6/MM6MapTransition.h), [MM6MapVisit.h](../src/Engine/MM6/MM6MapVisit.h),
  [MM6ParkedMapSection.h](../src/Engine/MM6/MM6ParkedMapSection.h),
  [MM6MapTimerSection.h](../src/Engine/MM6/MM6MapTimerSection.h),
  [MM6VisitedMapCache.h](../src/Engine/MM6/MM6VisitedMapCache.h)
- [MM6OutdoorWire.h](../src/Engine/MM6/MM6OutdoorWire.h), [MM6OutdoorSky.h](../src/Engine/MM6/MM6OutdoorSky.h),
  [MM6OutdoorBorderTravel.h](../src/Engine/MM6/MM6OutdoorBorderTravel.h),
  [MM6OutdoorBorderEligibility.h](../src/Engine/MM6/MM6OutdoorBorderEligibility.h),
  [MM6OutdoorBorderConfirmation.h](../src/Engine/MM6/MM6OutdoorBorderConfirmation.h),
  [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp)
- [MM6IndoorSnapshot.h](../src/Engine/Snapshots/MM6IndoorSnapshot.h),
  [MM6IndoorWorldPlan.h](../src/Engine/MM6/MM6IndoorWorldPlan.h),
  [MM6IndoorFaceOverrides.h](../src/Engine/MM6/MM6IndoorFaceOverrides.h),
  [MM6IndoorFaceLiveOwner.h](../src/Engine/MM6/MM6IndoorFaceLiveOwner.h), [Indoor.cpp](../src/Engine/Graphics/Indoor.cpp)
- [MM6Decorations.h](../src/Engine/MM6/MM6Decorations.h), [MM6Archive.h](../src/Engine/MM6/MM6Archive.h),
  [MM6PrivateMapSource.h](../src/Engine/MM6/MM6PrivateMapSource.h)
- [MM6ChestPlacement.h](../src/Engine/MM6/MM6ChestPlacement.h), [MM6ChestGridWire.h](../src/Engine/MM6/MM6ChestGridWire.h),
  [MM6ChestTrap.h](../src/Engine/MM6/MM6ChestTrap.h), [MM6ChestTrapDecision.h](../src/Engine/MM6/MM6ChestTrapDecision.h),
  [MM6ChestDisarmScore.h](../src/Engine/MM6/MM6ChestDisarmScore.h),
  [MM6ChestTypeLayout.h](../src/Engine/MM6/MM6ChestTypeLayout.h),
  [MM6ChestSeedAndOwnership.h](../src/Engine/MM6/MM6ChestSeedAndOwnership.h), [Chest.cpp](../src/Engine/Objects/Chest.cpp)
- [MM6TransitionDialog.h](../src/GUI/UI/MM6TransitionDialog.h), [GameConfig.h](../src/Application/GameConfig.h)
  (`gameplay.fix_*`)
- Game tests: `GameTests_MM6MapTravel.cpp`, `GameTests_MM6MapVisit.cpp`, `GameTests_MM6Transition.cpp`,
  `GameTests_MM6Chest.cpp`, `GameTests_MM6Decorations.cpp`, `GameTests_MM6WallPlate.cpp`, `GameTests_MM6Dungeons.cpp`,
  `GameTests_MM6MapSweep.cpp` in [test/Bin/GameTest](../test/Bin/GameTest)
- Tools: [run_mm6_map_sweep.py](../tools/run_mm6_map_sweep.py), [mm6_map_name_manifest.py](../tools/mm6_map_name_manifest.py)

See also: [Events](mm6-events.md), [Houses](mm6-houses-and-services.md), [Save format](mm6-save-format.md), [Combat](mm6-combat-and-monsters.md), [GrayFace](mm6-grayface-patch-reference.md)
