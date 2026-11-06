# MM6 monster projectile source gap

Status: F4 retained-table and consumer audit, 2026-11-06. No projectile mapping or effect behavior changed.

The F3-matched retained `monsters.txt` has 82 active nonempty projectile cells: 55 primary and 27 secondary. The inherited `ParseMissleAttackType` recognizes 36 of them and returns `MONSTER_PROJECTILE_NONE` for 46: 17 Electricity, ten Cold, six Poison, four Magic, three Rock, three Dagger and three flaming-arrow abbreviations. `NONE` is also the engine's melee/no-projectile sentinel, so the fallback can change attack selection before projectile rendering or damage. The [F2 combat audit](MM6_COMBAT_SEMANTICS_F2.md) recorded the same 46-cell gap.

Some labels suggest familiar projectiles, but the current enum and sprite lookup have no explicit MM6 Magic, Rock or Dagger contract. Mapping all seven fallback labels to inherited MM7 projectile IDs would guess their sprites, travel, impact and resistance behavior. The current monster damage and spell-effect gates do not prove attack-choice behavior for a row whose missile type already became `NONE`.

Next F4 work: retain a title-specific projectile label for every active primary/secondary cell before the inherited enum fallback; join each label to the MM6 object/sprite resource table and original guide or executable evidence; then route AI ranged selection, launch, impact and damage through a title-specific adapter. An unknown active label should reject source loading, and every one of the 82 cells needs a checked disposition. F5 must inspect representative Electricity, Cold, Poison, Magic, Rock, Dagger and flaming-arrow shots, including sound and save/re-entry. MM7's parser and projectile path should remain independent.
