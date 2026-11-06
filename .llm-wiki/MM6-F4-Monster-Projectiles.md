---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, monsters, projectiles]
---

# MM6 F4 Monster Projectiles

The [projectile packet](../project/MM6_F4_MONSTER_PROJECTILE_GAP.md) identifies 46 of 82 active retained projectile cells that formerly became the inherited `NONE` sentinel. All 82 now retain one of ten MM6 source types, so AI can distinguish ranged from melee intent. Their launch is gated before inherited MM7 sprite and impact creation. The exact affected monster IDs remain in the ignored F3 combat register.

Nine labels now have checked MM6 object rows in the retained 232-row object table and point to named `msl01`–`msl09` first frames among 6,455 retained sprite frames. The [offline checker](../scripts/mm6_f4_check_projectile_frames.py) matches all 46 sequence frames to retained sprite archive names. Dagger has a `knifid` sprite family but no proved projectile object. The next agent should inspect rendered frames, timing and octants and prove launch, travel, impact and damage before enabling the projectile path. No live projectile behavior is accepted yet.
