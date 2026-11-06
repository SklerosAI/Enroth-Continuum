# MM6 monster projectile source gap

Status: F4 title-specific source labels and launch gate, 2026-11-06. MM6 projectile sprites and impacts remain unverified.

The F3-matched retained `monsters.txt` has 82 active nonempty projectile cells: 55 primary and 27 secondary. The inherited `ParseMissleAttackType` recognized 36 of them and returned `MONSTER_PROJECTILE_NONE` for 46: 17 Electricity, ten Cold, six Poison, four Magic, three Rock, three Dagger and three flaming-arrow abbreviations. `NONE` is also the engine's melee/no-projectile sentinel, so that fallback changed attack selection before projectile rendering or damage. The [F2 combat audit](MM6_COMBAT_SEMANTICS_F2.md) recorded the same 46-cell gap.

The MM6 loader now maps all ten active source labels to distinct title-specific projectile IDs; an unknown active label rejects table loading, and zero-chance secondary placeholders remain `NONE`. The offline native test loaded all 173 monster rows and counted each of the 82 active cells by label. MM6 AI now sees a non-`NONE` type for those ranged attacks, and `AI_RangedAttack` rejects MM6 launch before inherited sprite/object creation. MM7 retains its original parser and launch path. This fixes source and ranged-choice classification while deliberately keeping projectile gameplay closed.

Some labels suggest familiar projectiles, but the current enum and sprite lookup have no explicit MM6 Magic, Rock or Dagger contract. Mapping source labels to inherited MM7 projectile IDs would guess their sprites, travel, impact and resistance behavior. The monster damage and spell-effect gates do not establish projectile visuals or effects.

Next F4 work: join each of the ten source labels to the MM6 object/sprite resource table and original guide or executable evidence; then implement launch, travel, impact and damage through a title-specific adapter. F5 must inspect representative Electricity, Cold, Poison, Magic, Rock, Dagger and flaming-arrow shots, including sound and save/re-entry. MM7's parser and projectile path should remain independent.
