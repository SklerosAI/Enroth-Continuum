---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, monsters, spells]
---

# MM6 F4 Monster Spells

The [monster spell packet](../project/MM6_F4_MONSTER_SPELL_NAMES.md) maps all 20 active retained monster spell names to MM6 spell IDs. All 55 active cells now retain a nonempty ID; 13 previously became none and 15 inherited matches used a different numeric ID. The MM7 parser is unchanged.

This is source routing. The MM6 monster effect guard still rejects unverified casts before inherited MM7 effects. The ignored F3 spell-effect report has exact per-monster joins and hashes; an agent should use it to prove rank, target, damage, resistance, sound and save behavior before enabling a cast.
