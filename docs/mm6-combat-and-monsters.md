# MM6 combat and monsters

How this port loads MM6 monsters and runs fights between them and the party, where MM6 differs from MM7, and which
GrayFace combat fixes are in.

## Approach

Combat runs through the shared `Actor.cpp`, `Character.cpp` and `TurnEngine.cpp`. MM6 branches either call a rule in
`src/Engine/MM6/` (`MM6CombatRules.h`, `MM6Monster*.h`) or pass through to shared code where MM6 and MM7 agree. Every
damage, spell and special-attack path that has not been checked against MM6.exe throws instead of running MM7 rules:

- `requireVerifiedPartyAttack`, `requireVerifiedMonsterHit` and `requireVerifiedMonsterOnMonsterHit` in
  `MM6CombatRules.h` run before the shared damage functions touch any state or the RNG.
- `isVerifiedMonsterSpell` (`MM6MonsterSpellEffect.h`) lists the monster spells with a checked launch, impact, damage
  and element.
- `requireVerifiedSpellEffect` (`MM6SpellEffectGuard.h`) is the same gate for player and event spells.

`tools/run_mm6_combat_sweep.py` runs `MM6MapSweep.DISABLED_MonstersAttackParty` on every map, one headless process per
map, and lists every throw with the monster that caused it.

## Monster records

`MonsterStats::Initialize` (`src/Engine/Objects/Monsters.cpp`) reads MM6 `monsters.txt` (173 rows) and rearranges its
columns into the shared `MonsterInfo`:

- MM6 puts the picture name before the display name.
- MM6 monsters have one spell. The second spell slot stays empty.
- MM6 has six resistance columns: Fire, Electricity, Cold, Poison, Physical, Magic. They are kept as they are in
  `MonsterInfo::mm6Resistances`. The shared fields also get them, with the one Magic value copied into all five MM7
  non-elemental schools.
- Attack damage labels are `Phys`, `Fire`, `Elec`, `Cold`, `Pois`, `Magic`, `Ener` (`parseMonsterDamageLabel`). An
  unknown label throws. A secondary attack with chance 0 has `0` in its type cell and is ignored.
- The shared runtime keeps MM6 Electricity, Cold and Poison in its Air, Water and Earth channels (`DAMAGE_AIR`,
  `DAMAGE_WATER`, `DAMAGE_EARTH`). Characters keep Poison resistance in their Body fields.
- Missile cells are matched per column against nine names each (`monsterProjectileType`). The two columns spell some
  names differently (`ArrowF` and `FireAr`, `Pois` and `Poison`). A cell that matches nothing leaves the attack melee.
- Special attack cells carry an `x<level>` suffix. The name is the first of 23 substrings found in the lowercased cell
  (`kSpecialAttackNames`), including the data's own spelling `affraid`.
- Spell cells map to MM6's 99 spell ids through `mm6::monsterSpellId` (`MM6MonsterSpells.h`), which accepts the data's
  misspellings.
- Attack preference `M` means male in MM6 (Monk in MM7). Class preferences match MM6's six three-tier class families
  (`matchesClassAttackPreference` in `MM6MonsterPreferences.h`).

MM6 `dmonlist.bin` has 173 records of 148 bytes (`MonsterDesc_MM6`) with no tint field. MM6 monsters get no tint, since
the outdoor billboard path reads a non-zero tint alpha as additive blending, which made every outdoor actor draw
see-through.

Monster ids come in families of three tiers (A, B, C), so `(id - 1) / 3` is the family. The shared
`MonsterEnumFunctions` helpers have no MM6 branch and use MM7 numbering, so MM6 code computes families itself.

## Actor identity

Map actors live in `.ddm` and `.dlv` deltas as native 548-byte records (`MM6OutdoorActorAppendCodec.h`). The monster id
is the one-byte field at offset 52, within 1 to 173. A separate 16-bit field at offset 118 is not the monster id.
Bytes 80 to 85 are a per-actor copy of the six resistances (Fire, Electricity, Cold, Poison, Physical, Magic). An actor
may override its table row there (`MM6NativeActorResistanceBytes.h`).

On map load the original gives a street NPC to each actor of the three peasant families (40, 41 and 44, the Peasant F1,
F2 and M1 families) whose record has a non-zero NPC link of either sign, and clears the link of every other actor
(`MM6NativeActorNpcPolicy.h`).

Saved actor PIDs and the active and spawned actor sections are part of the EC6 save. See
[MM6 save format](mm6-save-format.md).

## Hostility

MM6 has no `hostile.txt`. Whether a monster hunts the party comes from the `monsters.txt` hostility column, which is
non-zero for nearly every monster. `Actor::_SelectTarget` reads it under MM6. `HostilityTable::InitializeMm6` makes
every monster-to-monster relation friendly, since MM6 faction behaviour is not mapped.

The engine starts every actor at runtime hostility friendly until it notices the party, while MM6 reads the actor's
own hostility byte. `mm6::isMonsterHostileToParty` (`MM6PartyHostility.h`) counts a monster hostile when either its
runtime or its table hostility isn't friendly. The rest check, the alert gems and the minimap dots use it.

Alert gems (MM6.exe 0x401134): yellow within 5120 units of a hostile monster, red within 307.2.

## Peasants, reputation and families

MM6 has no fines and no fine award. Hurting and killing monsters works on reputation and on monster families
instead:

- **Death** (0x403050). Outside an Arena fight, the death of a monster whose `monsters.txt` hostility is 0 costs 100
  reputation, whoever dealt the blow. The Arena flag is the byte at 0x908DBD, set when a fight starts and cleared on
  map load. In the shipped `monsters.txt` only the Peasant F1, F2 and M1 families (IDs 121 to 126 and 133 to 135) have
  hostility 0. `mm6::reputationAfterMonsterDeath`, called from `Actor::Die`. `Actor::ApplyFineForKillingPeasant` does
  nothing under MM6.
- **Hurt** (0x403730). A monster whose own hostility byte isn't 4 turns hostile, and that costs 50 reputation. The port
  charges it when `mm6::isMonsterHostileToParty` says the monster wasn't hostile. `mm6::reputationAfterPain`, called
  from `Actor::AI_Pain`.
- **Bystanders** (0x432550, called from both). Monsters within 4096 units turn hostile: all three peasant families
  when the victim is one of them, otherwise only the victim's own family. `mm6::bystanderTurnsHostile`, called from
  `Actor::AggroSurroundingPeasants`.
- MM6 has no reagent drops. MM7's chance of one on a monster's death is off under MM6.

Outdoors, the actor mover (0x461590, family checks at 0x4615FB and 0x46230B) keeps the Water Elemental (58 to 60) and
Sea Serpent (151 to 153) families on water the way it keeps every other monster on land: a step from water onto land
is undone and the monster turns away. `mm6::keepsToWater`, used in the water check of `UpdateActors_ODM`.

The shared MM7 type helpers (`monsterTypeForMonsterId`, `isPeasant`, `supertypeForMonsterId`, `sexForMonsterType`)
map an MM6 family onto whatever MM7 type has its number, so MM6 ID 115 onwards (oozes, rats, sea serpents, the hive
bosses) lands on MM7's peasants. MM6 paths that still reach them are listed in [open questions](open-questions.md).

## Monster attacks

`mm6::monsterAttackDamage` (`MM6MonsterAttack.h`) rolls the attack's dice and adds the first attack's bonus. MM6 adds
the first attack's bonus to the second attack too.

Damage that reaches a character goes through `mm6::characterIncomingDamage` (`MM6CombatRules.h`):

- Physical passes unchanged.
- Other channels roll `resistedDamage`. The hit is halved up to four times while `random(luck bonus + resistance + 30)`
  is at least 30. Unlike MM7, the roll runs even when luck and resistance add to nothing.
- Resistance for Fire, Electricity, Cold and Poison is base plus bonus plus items plus the party protection buff plus
  20 for a hired Enchanter. Magic adds only its party buff. Energy has no resistance.

Objects a monster launches hit through `damageCharacterFromMonsterObject`. Only arrows (object 500) and the robot laser
(590) roll to hit, and only they are halved once each for the Shield buff, any Shielding item, artifact 408 worn as
armour, and artifact 423 in either hand. A cursed monster misses half its arrows.

### Special attacks

Only the first attack can carry the special attack. It fires when `random(100) < monster level * special level`.
`mm6::receiveSpecialAttack` then follows the original's "bad thing" routine:

- The character escapes when `random(save + luck bonus + 30) >= 30`.
- The save is a stat bonus for most effects (Personality for curse, Endurance for weakness, sleep, fear, drunk,
  disease and unconscious, Intelligence for insanity, Accuracy for theft). Poison uses Poison resistance. Paralysis,
  death, stone, eradication, aging and mana drain use Magic resistance.
- Break and steal first pick a random item and do nothing without one. Breaking saves against three times the item's
  rarity plus damage modifier. Theft takes the item and the thief keeps nothing.
- Warding items (Alarms, Sanity, Antidotes, Immunity, Freedom, Medusa and two artifacts) stop their effect after a
  failed save.
- Eradication checks the Unconscious condition, not Eradicated, which is what MM6.exe does.

### Missiles

| Missile | Object | Impact |
|---|---|---|
| Arrow | 500 | Damages what it hit and disappears |
| Flaming arrow | 510 | Same as arrow |
| Fire, Electricity, Cold, Poison, Magic, Rock | 520, 530, 540, 550, 570, 580 | Damages what it hit and turns into its explosion |
| Energy | 560, or 590 for the robot family | Same as above, the robot laser explodes as Energy |

`monsterObjectImpact` (`MM6MonsterProjectile.h`) classifies every object a monster can launch: arrows, missiles that
explode on what they hit, Fireball that damages everyone around it, Meteor Shower that flies through monsters, and
Finger of Death that can kill outright. Anything else is unverified and throws.

## Monster spells

Verified monster spells: 2, 4, 6, 9, 11, 18, 24, 26, 28, 37, 58, 65, 70, 76, 80, 91 and 95. That is the projectile
spells except Paralyze and Toxic Cloud, plus Meteor Shower (9), Dispel Magic (80) and Mass Curse (91).

- Damage uses MM6's monster table, not the player formula. Most projectile spells roll one d2, the rest one d2 per
  skill point, plus the monster's first attack bonus (`monsterSpellDice`).
- The element is the spell's own `spells.txt` damage tag. Vanilla MM6 dealt Fire for every monster spell. This is the
  GrayFace fix.
- Projectiles launch from half the monster's height. The skill goes onto the object as MM6's skill byte, level plus
  0x40 for Expert or 0x80 for Master.
- Meteor Shower works only outdoors and drops 8 meteors at Normal, 16 at Expert and 12 at Master from above the party.
- Dispel Magic ends every party buff and the buffs of each character who fails a Magic save.
- Mass Curse tries the curse special attack on each character who can act.

Paralyze (86) and other unverified monster spells throw. GrayFace's Paralyze fix is due when the spell is admitted.

## Spawning

Map encounters come from the three encounter slots in `mapstats.txt`. MM6 cells name either a compact internal family
name or a variant's display name. `mm6::resolveEncounterMonster` (`MM6EncounterMonster.h`) resolves the A, B or C
variant and returns invalid on a missing or ambiguous match, and the spawner throws then, rather than substitute an
unrelated monster.

The spawner follows MM6.exe 0x455910. A map's level odds cap at row 4 of the A, B and C table, 30%, 40% and 30%, where
MM7 goes on to row 5. The first monster of a group stands on the spawn point and each next one on a random point of a
ring around it, 128 outdoors and 64 indoors, in MM6.exe's 16.16 fixed point (`MM6EncounterPlacement.h`). Outdoors the
ring point isn't checked. Indoors a point outside the spawn point's sector, or with no floor within 1024 of the spawn
point, puts the monster on the spawn point instead. A spawned monster keeps its `monsters.txt` hostility and is not
made an aggressor, rest encounters included. Map load keeps every monster's hostility as MM6.exe does (0x455703 indoors,
0x46E880 outdoors), where MM7 resets it to friendly. The GrayFace patch doesn't touch the spawner.

The event opcode `SummonMonsters` (19) gives encounter slot 1 to 3, tier 1 to 3, a count and a position, with no group
or name. See [MM6 events](mm6-events.md).

A rest encounter can spawn only while some monster on the map can act and isn't a peasant (`restEncounterCanSpawn`,
MM6.exe 0x454C0E). Rest is refused near hostile monsters by MM6's own rule, see
[houses and services](mm6-houses-and-services.md).

When a monster dies with no dead sprite it is removed and drops its carried item. `mm6CarriedItem` builds that item
as MM6.exe 0x4217BD does.

Clicking a corpse loots it (MM6.exe 0x421670): gold from the treasure dice, then the carried item, or else a random
item with the treasure chance from MM6's item generator, see
[characters, items and spells](mm6-characters-items-spells.md#generation).

## Party attacks

Melee and bow to-hit and damage use the shared formulas. MM6-specific parts:

- Weapon special enchantments other than Antique throw (`requireVerifiedPartyAttack`). Elemental enchantments are not
  admitted yet.
- Mordred heals, Hades, Ares and Artemis add elemental damage, and Conan doubles against devils and dragons, see
  [artifacts and relics](mm6-characters-items-spells.md#artifacts-and-relics). Other weapons roll no extra damage.
- Blasters deal Energy, which no MM6 monster resists. MM7 treats blasters as physical.
- Player attack spells use `mm6::playerSpellDamage` and the spell's `spells.txt` damage tag (`MM6PlayerSpell.h`). Spells
  that roll to hit add 5 per spell level to the roll, and a miss says nothing.
- Damage to a monster goes through `mm6::monsterIncomingDamage`. A resistance of 200 or more is immunity. Lower
  resistance runs the same halving roll with no luck bonus.
- An object an event launched has no caster. Its explosion and its hits on monsters are ignored, as in the original.
- Clubs and empty or broken hands swing with the mace sound (MM6.exe jump table at 0x42A408).
- Wands throw.

## Recovery and turn-based mode

- Attack recovery takes the slower weapon when dual wielding (the MM7 rule, which is also the GrayFace fix).
- `gameplay.fix_recovery_enchant` (default on) makes "Increases rate of Recovery" divide each new recovery by 1.1,
  rounded, in real time and in turn-based mode. Off, MM6.exe's own per-frame code runs recovery 50% faster in real time
  only.
- A hit that Endurance reduces to zero recovery doesn't switch the active character.
- In turn-based mode MM6 has no party movement phase. After the party acts, the monsters move. MM6 ranks every
  character by attack recovery for initiative, without MM7's head start for ready characters, so about a third of the
  monsters act before the party when combat starts.
- MM6.exe advances 100 game seconds (426.67 ticks) per round. With `gameplay.fix_turn_based_turn_time` (default on) a
  round advances 50 game seconds (213 ticks), as GrayFace does. Monster movement per round doesn't change, it runs on
  the 64-tick AI timer.
- The turn indicator uses MM6's `newhand1` and `newglas1` sprites.

## Party death

A character at HP of minus its max HP or below dies on the next frame, and at 0 or below falls unconscious. MM7's rule
adds base Endurance instead. The party dies when no character can act, outside the rest screen (shared rule).

`mm6::respawnDeadParty` (`MM6PartyDeath.cpp`), called from `Game.cpp`, is MM6's counterpart of the shared
`GAME_STATE_PARTY_DIED` handler. During the Hive finale it plays the losing ending instead.

1. Hirelings leave, the losing movie plays, and the map the party died on is saved before time passes.
2. Each character gets the deaths award. The death count goes up and a week passes with no regeneration.
3. Gold is lost, or halved under an active Guardian Angel. All buffs and conditions are cleared. Every character has
   1 HP, or half or full HP under Guardian Angel at Expert or Master.
4. The party stands at the New Sorpigal respawn point on `oute3`, loading it if needed.
5. Every monster goes back to its start position with full HP and takes its hostility from the monster table again.
6. A character says the "cheated death" line.

MM6 buffs keep their original slot numbers in a few places (Guardian Angel is party buff 8, monster Curse is actor buff
2), which the code marks where it relies on them.

## GrayFace combat fixes

Landed, matching the GrayFace patch:

| Fix | Where |
|---|---|
| Monster spells deal their own element, not Fire | `monsterSpellDamageType` |
| Negative resistance no longer divides by zero | `resistedDamage` guards the divider |
| Second weapon doesn't double recovery | Shared MM7 rule |
| Recovery enchantment works, at +10% | `gameplay.fix_recovery_enchant` |
| Relic extra damage goes through the monster's resistance, none to a stoned monster | `gameplay.fix_bonus_damage_resistance` |
| Scholar gives +5% experience | `Party::GivePartyExp` |
| Turn-based round is 50 game seconds | `gameplay.fix_turn_based_turn_time` |
| Party can't hit a monster in its dying animation | Shared `CanBeDamaged` checks |
| Zero-recovery hit keeps the active character | `SetRecoveryTime`, `switchToNextActiveCharacter` |
| Progressive dagger triple damage, 1% per skill point | `Character.cpp` |
| Clubs swing with the mace sound | Port bug against MM6.exe, no flag |
| Arms Master, Weapons Master and Squire points shorten Expert sword, axe and bow recovery | `gameplay.fix_weapon_skill_hirelings` |
| Armor and shield recovery penalty halved at Expert, gone at Master | Port bug against MM6.exe, no flag |

Not yet: monster Paralyze, elemental weapon enchants through the monster resistance roll (the relics have it), Finger of Death experience for
the party, and the lava drop limit for monsters. That one hasn't reproduced, MM6.exe's
walking edge check, ported in `ProcessActorCollisionsBLV`, already keeps a walking monster from stepping more than 100
units down. Spell-side fixes (condition cures, Water Walk drain, stronger shorter buffs, Quick Spell school) are on
[characters, items and spells](mm6-characters-items-spells.md). The full list is on the
[GrayFace patch reference](mm6-grayface-patch-reference.md).

## Source

- [MM6CombatRules.h](../src/Engine/MM6/MM6CombatRules.h), [MM6MonsterAttack.h](../src/Engine/MM6/MM6MonsterAttack.h),
  [MM6MonsterDamage.h](../src/Engine/MM6/MM6MonsterDamage.h),
  [MM6MonsterProjectile.h](../src/Engine/MM6/MM6MonsterProjectile.h),
  [MM6MonsterSpecialAttack.h](../src/Engine/MM6/MM6MonsterSpecialAttack.h),
  [MM6MonsterSpellEffect.h](../src/Engine/MM6/MM6MonsterSpellEffect.h),
  [MM6MonsterSpells.h](../src/Engine/MM6/MM6MonsterSpells.h),
  [MM6MonsterPreferences.h](../src/Engine/MM6/MM6MonsterPreferences.h)
- [MM6EncounterMonster.h](../src/Engine/MM6/MM6EncounterMonster.h),
  [MM6MonsterRules.h](../src/Engine/MM6/MM6MonsterRules.h),
  [MM6PartyHostility.h](../src/Engine/MM6/MM6PartyHostility.h),
  [MM6NativeActorNpcPolicy.h](../src/Engine/MM6/MM6NativeActorNpcPolicy.h),
  [MM6NativeActorResistanceBytes.h](../src/Engine/MM6/MM6NativeActorResistanceBytes.h),
  [MM6SpellDamageTag.h](../src/Engine/MM6/MM6SpellDamageTag.h),
  [MM6SavedActorPidPolicy.h](../src/Engine/MM6/MM6SavedActorPidPolicy.h)
- [MM6PartyDeath.cpp](../src/Engine/MM6/MM6PartyDeath.cpp), [Monsters.cpp](../src/Engine/Objects/Monsters.cpp),
  [Actor.cpp](../src/Engine/Objects/Actor.cpp), [TurnEngine.cpp](../src/Engine/TurnEngine/TurnEngine.cpp),
  [HostilityTable.cpp](../src/Engine/Tables/HostilityTable.cpp)
- Unit tests in [src/Engine/MM6/Tests](../src/Engine/MM6/Tests): `MM6CombatRules_ut.cpp`, `MM6MonsterAttack_ut.cpp`,
  `MM6MonsterDamage_ut.cpp`, `MM6MapStatsEncounterColumns_ut.cpp`, `MM6PartyDeath_ut.cpp`
- Game tests: `GameTests_MM6Combat.cpp`, `GameTests_MM6FirstCombat.cpp`, `GameTests_MM6Dungeons.cpp`,
  `GameTests_MM6PartyDeath.cpp`, `GameTests_MM6GrayFaceShared.cpp`, `GameTests_MM6MapSweep.cpp`,
  `GameTests_MM6MonsterFamilies.cpp` in
  [test/Bin/GameTest](../test/Bin/GameTest)
- [run_mm6_combat_sweep.py](../tools/run_mm6_combat_sweep.py)

See also: [Characters, items, spells](mm6-characters-items-spells.md), [Events](mm6-events.md), [Save format](mm6-save-format.md), [GrayFace](mm6-grayface-patch-reference.md)
