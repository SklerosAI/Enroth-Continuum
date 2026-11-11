# MM6 event spells handoff

- Owner: Claude Code lane, reviewed by Claude lead. Integrated by the lead.
- Branch: `claude/mm6-event-spells` in the `mm7-monster-combat` checkout, commits `f129d1769` (event spells) and `3494b69b9` (Ice Bolt impact), on top of the damage channels. Cherry-picked onto `master`.
- Shared code, each change MM6-gated: `eventCastSpell` in `Spells.cpp` hands MM6 casts to `mm6::launchEventSpell`, the sprite-caster branch of `DamageCharacterFromMonster` routes MM6 hits, `ItemDamageFromActor` returns early for MM6, and `processSpellImpact` treats MM6 sprite 3050 as a bolt. The rest is new MM6 code in `src/Engine/MM6/MM6EventSpell.h`.

## Original behavior (from the MM6 executable)

Opcode 21 reads the spell, a 0-based mastery, the level and two points. A zero target point aims at the party. The caster is a sprite object. Spells 6, 28, 32 and 90 launch one projectile, 15 launches 3, 5 or 7 sparks over 60 degrees by mastery, and 98 does nothing in the original. Damage ignores mastery, masks the level to 6 bits, and uses the spell's damage channel through the MM6 damage channels: Fireball level d6 over radius 512, Sparks level + 2, Ice Bolt level d7, Ice Blast 12 + level d2 per shard, Toxic Cloud 25 + level d10. Event projectiles never hurt monsters. The guard still refuses every other spell ID.

## Checks

- Unit `MM6EventSpell` (4). Game tests: the sci-fi Sparks timer, the cd1 Fireball timer, a cd2 poison plate and a cd2 Ice Bolt plate, each taping HP and sprites. Revert-and-fail.
- Sweep of 25 event-spell maps: clean with actors 15 before, 22 after. Maps refusing an event spell 17 before, 6 after, none on the main quest. The remaining refusals are spells 12, 35, 46, 83 and 91 on d01, d04, d08, d13, znwc and zddb08.
- Unit tests 1,096 passed. All nine `MM6CombatHeadless` passed alone. MM6 suite 94 passed. MM6 first-map gate passed with the MM7 controls. MM7 in-process 107 passed. Full MM7 headless parallel 540/540. `check_style` clean.

## Open

The town-guard Fireball timers against a flying party are untested, since nothing makes the party fly yet. The t7 plates were untestable on this branch before the decoration parser fix, which is now on `master`. Length-4 and length-5 CastSpell records in a few scripts fail the parser's size check and are ignored.

Privacy: IDs, enum names, addresses and map stems only.
