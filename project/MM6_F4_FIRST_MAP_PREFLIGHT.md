# MM6 first outdoor map preflight — F4

Status: **read-only headless resource preflight staged; linked execution and live map load pending** (2026-11-06).

## Route and checked resources

`Game::loop()` accepts party creation, sets an MM6 pending transition to `oute3.odm`, then calls `saveNewGame()` before `gameLoop()`. That save entry point remains guarded. If it completes later, `gameLoop()` loads portraits and calls `DoPrepareWorld()`. The first map table ID is outdoor under the current dispatch, so the outdoor loader reads its base and `.ddm` delta, then `_461103_load_level_sub()` reaches the still-guarded MM6 chest generator before `onMapLoad()` and the first game frame.

The headless `FirstOutdoorResourcePreflight` reads only private data through the selected engine resources. It checks the table's first outdoor identity, the matching base and delta members, map event/string resources, the minimap image, the loading background, and the base map's sky field and bitmap. It deserializes the base map into a detached snapshot; it does not call `DoPrepareWorld()`, change live map state, invoke event scripts, or write to the installed profile. The lookup identifiers needed by the runtime appear in source and tests; no original bytes or private inventory is tracked.

The MM6 icon archive has one 640×480 loading background. `fullscreenLoadingResources` selects it only for MM6; MM7 still chooses one of its five existing names with the same random draw. MM6 has no corresponding `loadprog` image, so its full-screen loading view draws the background without the inherited progress strip. Progress events still advance internally. This is a presentation gap pending visual acceptance, not a claim that map loading works.

The first outdoor base map explicitly names an available sky bitmap. `OutdoorLocation::Load()` reconstructs that name, but the inherited weather branch replaced it with an unavailable MM7 default. The MM6 path now uses a nonempty sky name from the base map, while MM7 keeps its weather and randomization path. Other MM6 outdoor maps may have no base sky name, so this is a first-map binding, not a general sky policy.

## Live blockers and next acceptance

1. The guarded MM6 `saveNewGame()` must commit a complete `.ec6` transaction and seed `pMapDeltas` before outdoor loading. `OutdoorLocation::Load()` indexes the first `.ddm` in that map; an empty or incomplete delta table would throw before the chest call. This packet does not edit SaveLoad or its transaction.
2. The MM6 chest generator remains guarded in `_461103_load_level_sub()`. A detached resource preflight cannot bypass it or prove the map can finish loading.
3. The first map's reload events, spawned actors/items, music, HUD and first rendered frame need a bounded live scenario after the save/chest gates. The existing MM6 UI still has separate minimap-frame, NPC-arrow and other presentation gaps. Later boxed loading calls use an MM7 `turnhour` animation absent from MM6; the first full-screen path does not draw it.

The linked headless test should verify resource availability and exact title isolation. Once the save and chest owners open their guards, a monitored null-window scenario can assert map ID, delta presence, initialized outdoor state and a synthetic frame. Graphical, audible and save/reload acceptance remain separate.

See the [headless harness](MM6_F4_HEADLESS_HARNESS.md), [HUD packet](MM6_F4_HUD_RESOURCES.md), and [active-world identity packet](MM6_F4_ACTIVE_WORLD_PID_RESTORE.md).
