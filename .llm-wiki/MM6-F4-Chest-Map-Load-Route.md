---
type: synthesis
up: "[[MM6-F4-Chest-Live-Owner-Adapter]]"
tags: [mm6, f4, chest, map-load, headless]
---

# MM6 F4 Chest Map Load Route

The [source packet](../project/MM6_F4_CHEST_MAP_LOAD_ROUTE.md) describes the scoped MM6 chest join at `Engine::_461103_load_level_sub()`. Default MM6 still reaches the blocking generator guard, and MM7 still takes its inherited route. No production caller enables the scope.

`ScopedChestMapLoadOptIn` enables the route only on the loading thread for headless acceptance. The route requires a ready MM6 chest title state, valid map ID, item table, party, current decoded active-map sidecar and matching twenty-chest runtime. It scans party-owned 400–429 artifacts and calls the atomic live-owner handoff, which commits chests, copied sidecar, LCG and generated flags only after all checks succeed. A failed attempt falls back to the existing guard without touching these owners.

Three focused arm64 synthetic outdoor tests pass for scoped commit, default/foreign/missing/malformed rollback and a direct artifact capped by party ownership. `Engine.cpp` syntax passes. This is not a full `DoPrepareWorld` test. New-game autosave precedes first outdoor map load, so the save stage must prepare CHST and decoded `oute3.ddm` first. The headless agent owns integrated first-map and rollback acceptance; only after that result and first-open/save checks should normal MM6 enablement be considered.

See also: [MM6 F4 Chest Live Owner Adapter](MM6-F4-Chest-Live-Owner-Adapter.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [MM6 Headless Integration Policy](MM6-Headless-Integration-Policy.md).
