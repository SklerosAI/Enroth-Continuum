---
type: synthesis
up: "[[MM6-F4-Headless-Harness]]"
related: ["[[MM6-F4-HUD-Resources]]", "[[MM6-F4-Active-World-PID-Restore]]"]
tags: [mm6, f4, first-map, resources, headless]
---

# MM6 F4 First Map Preflight

The [public packet](../project/MM6_F4_FIRST_MAP_PREFLIGHT.md) traces new-game startup through the first outdoor loader and records a read-only headless resource preflight.

`Game::loop()` sets `oute3.odm` as the MM6 pending transition and calls guarded `saveNewGame()` before `gameLoop()`. The later `DoPrepareWorld()` path reads map events/strings, dispatches to the outdoor loader, reconstructs the base and a `.ddm` from `pMapDeltas`, and then reaches the still-guarded MM6 chest generator. The test checks the map-table dispatch, base/delta and event resources, the parsed reload-trigger count, minimap icon, one MM6 loading background, and a detached base-map sky field and bitmap. It does not enter the map, execute events or write a save.

The retained MM6 archive has a single full-screen loading background. `fullscreenLoadingResources` selects it only for MM6 and omits MM7's absent progress-strip texture; the MM7 variation and progress lookup remain exact. MM6's later boxed loading view has its background but no `turnhour` animation, so it skips that image rather than indexing an invalid animation ID. The first outdoor base map names an available sky bitmap. `OutdoorLocation::Load` now preserves a nonempty MM6 base sky name instead of replacing it with the unavailable MM7 default; the later outdoor setup and weather initialization do not overwrite its loaded pointer. This is proved for the first map only; other outdoor maps may need a title-specific sky fallback.

The first MM6 map's numeric ID aliases MM7 `MAP_SHOALS`. The inherited MM7 hireling block incorrectly hid follower UI and stopped dialogue, specialties and a spell route. All five consumers now use a title-aware helper: MM6 avoids this MM7 restriction, and MM7 retains it. Any genuine MM6-only restriction needs separate evidence.

The save transaction must populate `pMapDeltas` before load; the guarded chest generator then must finish. Reload-event execution, actor/item spawns, HUD, music, first frame and durable save/reload remain unaccepted. The linked headless preflight checks resources and the detached event parse only; graphical play needs the later F5 pass.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [HUD Resources](MM6-F4-HUD-Resources.md), [Active World PID Restore](MM6-F4-Active-World-PID-Restore.md).
