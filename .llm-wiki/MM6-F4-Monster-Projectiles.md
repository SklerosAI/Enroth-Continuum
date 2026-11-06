---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, monsters, projectiles]
---

# MM6 F4 Monster Projectiles

The [projectile gap packet](../project/MM6_F4_MONSTER_PROJECTILE_GAP.md) identifies 46 of 82 active retained projectile cells that currently become the inherited `NONE` sentinel. This can alter ranged versus melee AI selection before damage gates apply. The exact affected monster IDs remain in the ignored F3 combat register.

The next agent should preserve typed MM6 labels, join them to shipped sprite/object resources, and prove attack selection, launch and impact before enabling the projectile path. No live projectile behavior is accepted yet.
