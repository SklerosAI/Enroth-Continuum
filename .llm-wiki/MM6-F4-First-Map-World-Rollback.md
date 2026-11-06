---
title: MM6 First-Map World Rollback Audit
summary: The scoped EC6 save test has exact disk/cache/CHST rollback, while outdoor entry still mutates unjoined world owners; a small collection scratch is tested.
tags: [mm6, f4, save, world, rollback]
---

The [rollback audit](../project/MM6_F4_FIRST_MAP_WORLD_ROLLBACK_AUDIT.md) enumerates owners changed by `loadAndPrepareODM` and `DoPrepareWorld`. The monitored save callback already restores the exact primary/backup slot pair, 67-map cache and CHST owner after failure. It has not run the full outdoor loader.

`ScopedFirstMapWorldCollections` swaps the prior actor, sprite, chest and decoration containers into private storage and exposes empty scratch containers. It restores all four after a late synthetic fault, or keeps the new collections on explicit commit. This is the bounded safe subset. `pOutdoor` itself registers a season listener and owns terrain/resource pointers; Party placement, engine transition/event state, weather/camera/UI/audio and spawn/reload effects also lack a complete rollback. Do not call `loadAndPrepareODM` through the scoped save callback as an atomic transaction until those owners have a staged or snapshot policy and monitored failure tests. Normal MM6 SaveLoad remains guarded.
