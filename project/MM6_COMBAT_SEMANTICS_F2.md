# MM6 combat semantics: retained-row and consumer audit (F2)

Status: **source and retained-reference audit, 2026-11-05**. This extends the
[monster/combat specification](MM6_MONSTER_COMBAT_SPEC.md); it does not close F2,
perform the consolidated F3 original-file preflight, or establish a playable
encounter. The owned [Prima guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf)
is used as a behavior lead at printed pp. 28–30 (PDF pp. 36–38), and the
owned manual at PDF pp. 37–40 supplies the combat/recovery context. Shipped
data and observed behavior must decide discrepancies. No guide passage or
per-monster record is copied into Git.

## Evidence and reproducible private map

`scripts/mm6_probe_combat_semantics.py` reads only the already retained,
ignored `local-game-data/mm6/research-2026-11-05/icons/monsters.txt` and
tracked C++ source. It rejects a report path outside ignored
`local-game-data/mm6/`, asserts key source assumptions, records hashes, and
writes one private record for **every ID 1–173**. Each record joins MM6 picture
and display name, numeric family/tier, inherited MM7 family identity, stats,
attack preference, attack and projectile slots, secondary/spell chances,
special attack, treasure, AI/hostility, and all six resistance cells. The
retained `monster-rules-probe.json` adds per-label parser classifications;
`spell-effect-probe.json` adds MM6 spell-ID comparisons. Their exact records
remain ignored. Reproduce the new inventory with:

```sh
python3 scripts/mm6_probe_combat_semantics.py \
  local-game-data/mm6/research-2026-11-05 \
  local-game-data/mm6/research-2026-11-05/combat-semantics-probe.json
```

The probe found 173 contiguous 33-field rows with no structural errors,
**57 complete numeric triplets plus one two-row tail**. A numeric triplet is a
candidate family, not proof of semantic family, sprite, or spawn identity.
Active rows include 39 with a secondary-attack chance, 55 with a spell chance,
93 with a special-attack label, 68 with an attack preference, 145 with a
nonzero treasure cell, and 42 marked flying. The MM6 special-ability column
is zero for all 173 rows; inherited MM7 summon/multishot/explode parsing is
therefore not evidence of MM6 monster behavior. The 173 retained fixed
`dmonlist.bin` records are structurally described in the prior spec, but the
ID-to-picture/sprite and map-spawn joins still need F3 confirmation.

## Parser to consumer map and confidence

Confidence rates the **source diagnosis**, not the correctness of proposed
MM6 gameplay. "High" means the retained row and current source path directly
establish the gap; "medium" means a guide or cross-table join guides expected
behavior; "open" means a shipped executable or live encounter must settle it.

| Family | Retained and guide evidence | Current C++ consumer and F2 conclusion | Confidence / unresolved behavior |
| --- | --- | --- | --- |
| Identity, family, tier and sprite | 173 rows, 57 full triplets and one two-row tail; guide groups most variants in threes. | `MonsterStats::Initialize` loads numeric IDs and names; `MonsterEnumFunctions.h` computes family/tier from ID, but `MonsterEnums.h` starts with an MM7 family that differs from the first retained MM6 family. `MonsterEnumFunctions.cpp` uses that MM7 type for sex/race, arena, bounty, peasant, supertype and fixed drops. `Actor.cpp` uses supertype for behavior and drop rules. `MonsterList` and map-spawn wiring require an explicit MM6 ID join. | **High** for identity collision; **open** for every sprite/spawn and two-row-tail meaning. The private report shows the inherited type beside every MM6 ID for audit, not use. |
| Primary and secondary attacks | 212 active damage-type cells; 39 rows have a secondary chance. Guide pp. 29–30 describes typed attacks, chance and recovery. | `Monsters.cpp` parses dice and types; `Actor::special_ability_use_check` selects spell before secondary before primary, then `_43B3E0_CalcDamage`, `Character::receiveDamage` and projectile/hit paths apply effects. Existing parser report finds **49 active type cells fall to physical** and **12 `Ener` cells map to Earth**; 61 rows have either issue. | **High** for fallback and selection order; **medium** for guide-level intended chance/recovery; **open** for exact MM6 energy/magic damage/resistance and secondary recovery multiplier. |
| Missiles and collision | 82 active missile cells; guide distinguishes attack type from missile form. | `ParseMissleAttackType` feeds `Actor` ranged attack and `spriteForMonsterProjectile`; **46 cells become `NONE`** under the shared parser. A nonzero dice roll alone cannot create a visible, colliding missile. | **High** for missing mappings; **open** for every MM6 sprite, flight/collision, impact and area behavior. |
| Monster spells | 55 active spell cells/chances; guide p. 30 names spell use as a probabilistic action. | `ParseSpellType` and `ParseSkillValue` feed `Actor::special_ability_use_check`, `AI_SpellAttack`, `CalcSpellDamage`, and the shared spell dispatcher. **13 cells become `SPELL_NONE`**; the separate [spell effect audit](MM6_SPELL_EFFECT_F2.md) found **15 additional recognized labels with the wrong MM6 numeric ID**, leaving 27 with the same numeric ID. Matching IDs do not prove equal effect. | **High** for parser/ID mismatch; **open** for MM6 effect, mastery, target, resistance and sound/animation. |
| Special attack and character condition | 93 active labels; guide p. 29 says they augment a successful hit. | `ParseSpecialAttack` feeds `Character::ReceiveSpecialAttackEffect` after hit/damage. **11 labels become `NONE`**. Existing effect code uses MM7 condition resistance, break/steal item-category and state rules. | **High** for unrecognized labels; **open** for each MM6 condition check, severity/level, item theft/break selection, recovery and save persistence. |
| Resistance and immunity | Six MM6 channels: Fire, Electricity, Cold, Poison, Physical, Magic. Active nonzero cells respectively **106, 105, 106, 111, 86, 113**; `Imm` cells **18, 14, 13, 45, 5, 54**. Guide p. 30 names exactly these six channels. | `Monsters.cpp` maps first four into MM7 Fire/Air/Water/Earth, Physical into `resPhysical`, and repeats one MM6 Magic value into Mind/Spirit/Body/Light/Dark. `Actor::CalcMagicalDamageToActor` and `DoesDmgTypeDoDamage` switch on MM7 channels and default unknown damage to unresisted. There are no dedicated Magic/Energy cases. | **High** for six-column inventory and alias; **medium** for the guide's names; **open** for exact source damage-channel mapping, numeric formula and whether `Ener` is a distinct seventh attack channel. Preserve `Imm` until verified, not merely the value 200. |
| AI, preference and hostility | Retained AI labels cover four types; 68 rows declare preferences; 42 fly. Guide pp. 29–30 describes retreat thresholds, preferred targets and recovery. | `Monsters.cpp` parses AI/flags. `Actor.cpp` contains a shared retreat routine, target preference and attack choice; `HostilityTable::InitializeMm6` makes every monster-to-monster relation friendly while per-row hostility affects party targeting. The shared Wimp path flees immediately when hostile, while guide describes a damage threshold; Normal/Aggressive code uses different remaining-HP thresholds than guide wording. | **High** for source/guide discrepancy and friendly faction default; **open** for original AI thresholds, priority, map-specific factions, turn-mode timing and flying navigation. Do not implement guide prose blindly. |
| Random and fixed loot | 145 nonzero treasure cells. Guide pp. 28–29 separates gold dice, item chance, item level/type. | `Monsters.cpp::parseTreasure` stores dice/chance/level/type; `Actor::SetRandomGoldIfTheresNoItem`, corpse looting, and `ItemTable::generateItem` consume them. `Actor::Die` also calls MM7 `itemDropForMonsterType` and uses MM7 numeric family; `MM6ItemTable` and shared item categories still need a title-specific join. All active treasure suffixes are lexically recognized, **not** validated as correct generated items. | **High** for source paths/ID collision; **medium** for guide format; **open** for MM6 item ranges, fixed drops, gold/item independence, seed timing and save/re-entry behavior. |

## F4 implementation contract

1. Introduce MM6-specific monster identity and a validated 173-row manifest:
   `MonsterId -> MM6 family, tier/unique status, description, picture/sprite,
   spawn ID, supertype, faction, loot policy`. Keep MM7 definitions and saves
   separate. Treat the final two-row group explicitly, not as a fabricated C
   variant. Reject missing active joins before campaign use.
2. Decode every active attack, projectile, monster spell, special attack and
   treasure category through title-specific, exhaustive mappings. No unknown
   active label may silently become physical, no projectile, no spell, no
   condition or generic loot. Preserve exact source labels in private tests.
3. Define MM6 damage/condition resistance as a six-channel contract, including
   `Imm`, and decide Energy from original executable or witnessed behavior.
   Route both player and monster damage through it; do not reuse MM7 class,
   item or spell IDs solely because names match.
4. Define faction targeting, AI retreat and preference, action recovery and
   turn-mode selection from reconciled original evidence. Guide thresholds
   are test hypotheses; the current inherited Wimp and hostility defaults are
   not an MM6 specification.
5. Generate gold, random items and fixed drops with MM6 item IDs and category
   bands. Persist and reload actor HP/state, conditions, consumed corpse items
   and map actor deltas without rerolling loot. The [world field map](MM6_WORLD_FIELD_MAP_F2.md)
   is a prerequisite for actor serialization.

## F5 encounter and persistence scenarios

Use a reproducible local save and inspect the graphical view, status text,
sound, and changed save bytes. Representative cases must include: an opening
melee triplet, a second-attack monster, a ranged elemental/magic/energy user
with visible impact, a caster whose label currently falls to `SPELL_NONE`, a
recognized but shifted-ID caster, a status attack that alters a character and
persists, a flying target, a neutral/faction pair, and a creature with an
immunity. Exercise real-time and turn-mode selection and recovery. For loot,
kill and inspect gold, random item and any fixed drop; save, exit, re-enter and
confirm no reroll or duplicate corpse reward. Repeat at least one encounter
outside the starting region to cover a different family and map-spawn path.

## Remaining F2/F3 questions

- Reconcile the private 173-row ID/picture list with all retained map spawn
  references and 173 binary descriptions; verify the final two-row tail and
  special/unique IDs at the single F3 original-file pass.
- Determine exact MM6 `Magic`, `Ener`, elemental and condition resistance
  formulas and all active special-attack meanings from original source
  evidence. The guide's qualitative resistance scale is insufficient.
- Determine original AI retreat, faction relationships, preferred-target
  order, projectile semantics, fixed drops and gold/item generation where the
  retained table and current source do not prove them. Carry each unresolved
  behavior into offline F4 tests and visible F5 cases.

The [F3 preflight matrix](MM6_F3_PREFLIGHT_MATRIX.md) remains the authority for
the one original-resource confirmation. This packet supplies its monster
consumer/acceptance detail; it does not authorize an early archive read.
