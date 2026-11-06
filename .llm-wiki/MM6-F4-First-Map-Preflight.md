---
type: synthesis
up: "[[MM6-F4-Headless-Harness]]"
related: ["[[MM6-F4-HUD-Resources]]", "[[MM6-F4-Active-World-PID-Restore]]"]
tags: [mm6, f4, first-map, resources, headless]
---

# MM6 F4 First Map Preflight

The [public packet](../project/MM6_F4_FIRST_MAP_PREFLIGHT.md) traces new-game startup through the first outdoor loader and records a read-only headless resource preflight.

`Game::loop()` sets `oute3.odm` as the MM6 pending transition and calls guarded `saveNewGame()` before `gameLoop()`. The later `DoPrepareWorld()` path reads map events/strings, dispatches to the outdoor loader, reconstructs the base and a `.ddm` from `pMapDeltas`, and then reaches the still-guarded MM6 chest generator. The test checks the map-table dispatch, base/delta and event resources, minimap icon, one MM6 loading background, and a detached base-map sky field and bitmap. It does not enter the map or write a save.

The retained MM6 archive has a single full-screen loading background. `fullscreenLoadingResources` selects it only for MM6 and omits MM7's absent progress-strip texture; the MM7 variation and progress lookup remain exact. The first outdoor base map names an available sky bitmap. `OutdoorLocation::Load` now preserves a nonempty MM6 base sky name instead of replacing it with the unavailable MM7 default. This is proved for the first map only; other outdoor maps may need a title-specific sky fallback.

The save transaction must populate `pMapDeltas` before load; the guarded chest generator then must finish. Later boxed loading has an MM7 turn-hour animation absent from MM6. Reload events, actor/item spawns, HUD, music, first frame and durable save/reload remain unaccepted. The linked headless preflight checks resources only; graphical play needs the later F5 pass.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [HUD Resources](MM6-F4-HUD-Resources.md), [Active World PID Restore](MM6-F4-Active-World-PID-Restore.md).
