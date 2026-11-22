# MM6 characters, items and spells

The MM6 rules for the party, characters, items, potions, spells and awards: what the port implements, where each
rule lives in source, and which parts are still refused.

The MM6 port runs on the inherited MM7 engine types (`Party`, `Character`, `Item`, `SpellBuff`) and branches on
`engine->gameTitle() == GameTitle::MM6` where MM6 differs. MM6-only rules live in `src/Engine/MM6`,
`src/Engine/Spells/MM6*` and `src/Engine/Tables/MM6*`. A rule that isn't checked against the original executable is
refused with an exception or an error sound instead of falling back to MM7 behaviour. That fail-closed habit runs
through this whole page.

Rule tables read from `MM6.exe` are on [Rules from MM6.exe](mm6-executable-rules.md). Teaching skills, training,
selling spells and temples are on [Houses and services](mm6-houses-and-services.md).

## Classes

MM6 has 18 classes in six families of three tiers. A class id is `family * 3 + tier`, and only tier 0 can be picked at
creation (`mm6::isStartingClass`). The ids are stored in the shared `Character::classType`, whose MM7 enum names
don't apply: every MM6 branch converts it with `std::to_underlying` and uses the helpers in `MM6CreationRules.h`.

| Ids | Family |
|---|---|
| 0 to 2 | Knight, Cavalier, Champion |
| 3 to 5 | Cleric, Priest, High Priest |
| 6 to 8 | Sorcerer, Wizard, Arch Mage |
| 9 to 11 | Paladin, Crusader, Hero |
| 12 to 14 | Archer, Battle Mage, Warrior Mage |
| 15 to 17 | Druid, Great Druid, Arch Druid |

Spell points come from a class-dependent stat, as the original's `GetFullSP` dispatch has it (`mm6::manaAttribute`).
Knights have none, Clerics and Paladins use Personality, Sorcerers and Archers Intellect, Druids both.

Maximum HP is `hpBase[family] + hpFactor[class] × (level + Endurance bonus)` plus skill, item and full-health bonuses.
Maximum SP is the same shape with the SP arrays and the mana stat bonus. Both arrays come from the executable. The
combination with bonuses is marked provisional in `Character::GetMaxHealth` and `GetMaxMana`. Bodybuilding and
Meditation add `hpFactor` or `spFactor` times their skill bonus.

## Party creation

The creation screen (`UIPartyCreation.cpp` with MM6 branches in `Character.cpp`) follows the executable's creation
arrays:

- Picking a class (`Character::ChangeClass`) sets level 1, a random starting experience of 251 to 350, a birth year up
  to five years before `mm6::characterBirthYearBase` (1144), clears all skills and grants the family's two code-1
  skills at Novice. Stats reset to the family's starting stats.
- Every family's starting stats sum to 73. The party shares a pool of 50 bonus points. A stat can go up to 25 and down
  to two below its starting value.
- Each family has nine code-2 skills, of which the player picks two (`Character::GetSkillIdxByOrder`).
- HP and SP are filled after the starting items are handed out (`MM6CreationVitals.h`), the original's order.
- The game starts in 1165 (`mm6::calendarStartingYear`) on `oute3.odm`, New Sorpigal.

Starting items get native 28-byte records through `seedMM6CreationItems` (`MM6CreationItemSeed.h`), so the first save
can hold them. The original's Quick Start party is not implemented, the fixture was never extracted.

## Skills

MM6 has 31 skills and three masteries, Normal, Expert and Master. There is no Grandmaster, `mm6::masteryRank` throws
on it. The shared `Skill` enum agrees with MM6's order through id 29. MM6 skill 30, Learning, maps to the shared
`SKILL_LEARNING`.

| MM6 ids | Skills |
|---|---|
| 0 to 7 | Staff, Sword, Dagger, Axe, Spear, Bow, Mace, Blaster |
| 8 to 11 | Shield, Leather, Chain, Plate |
| 12 to 20 | Fire, Air, Water, Earth, Spirit, Mind, Body, Light, Dark |
| 21 to 30 | Identify, Merchant, Repair, Bodybuilding, Meditation, Perception, Diplomacy, Thievery, Disarm Traps, Learning |

A character can learn a magic school's spells only when the family's starting-skill code for that school is nonzero
(`mm6::classCanLearnSpellSchool` in `MM6SpellLearning.h`). An empty or broken hand swings with the mace sound, as
`MM6.exe` plays it.

## Conditions and recovery

- The 19 runtime condition clocks are kept as they are. Stats are evaluated against the first active condition in
  MM6's own priority order (`kActualStatConditionPriority` in `MM6ActualStatCondition.h`). MM6 has no Zombie entry
  there.
- A new day starts at 3 am (`mm6::isNewDay`, `MM6NewDay.h`). `mm6::startNewDay` decays reputation by 1%, counts the
  days without rest, and from the second day without rest makes everyone Weak, eats food or cuts HP. From the fourth
  day it can kill or drive characters insane. Conditions set there skip speech and item protection, as in the
  original. `gameplay.fix_new_day_at_month_start` also starts a day on a time jump across a month end.
- A cast's recovery comes from the executable's spell table. The "Increases rate of
  Recovery" enchantment (special 17) speeds real-time recovery by 50% in the original. With
  `gameplay.fix_recovery_enchant` it shortens every recovery by 10%, in real time and turn-based, as the GrayFace
  patch does.
- A turn-based round advances the clock by 50 game seconds with `gameplay.fix_turn_based_turn_time`, the original
  takes 100.

## Items

### Item records and ids

The MM6 item table holds ids 1 to 580 (`items.txt`, loaded by `ItemTable` with `MM6ItemTable.cpp`). Notable ranges:

| Ids | What |
|---|---|
| 1 to 399 | Regular items the random generator can pick |
| 64, 65 | Blaster and Blaster Rifle, value 0 |
| 160 to 162 | Herbs |
| 163 | Empty bottle |
| 164 to 188 | Potions, the `useitems.txt` axis covers 160 to 188 |
| 189 to 196 | Further bottle templates with no use rule, refused |
| 197 to 199 | Gold piles |
| 200 to 298 | Spell scrolls, one per spell in spell order (`MM6SpellItems.h`) |
| 300 to 398 | Spellbooks, one per spell in spell order |
| 400 to 429 | Artifacts and relics the generator can roll, tracked by 30 generated flags, see below |

These ranges overlap MM7's reagent and potion ids. Code that tests raw ids with MM7 predicates must check the title
first, see `MM6ConsumableRules.h`.

Every MM6 item carries its original 28-byte record in `Item::mm6RawRecord` (`mm6::ItemRecord28`): item number at +0,
standard bonus at +4, its strength at +8, special bonus or gold amount at +12, charges at +16, condition flags at +20,
body location at +24, max charges at +25, and two bytes the original keeps but the port doesn't interpret. The record
is the authority: the save writes it back, and the shop price reads it. An item that arrives without a record, or with
fields the bounded projections don't cover, can't be saved, see [Save format](mm6-save-format.md#what-is-and-isnt-saved).
Event item gifts (`MM6ItemEventWrite.cpp`) and monster carried items (`mm6CarriedItem` in `Actor.cpp`) build their
record the way the original does. So does every item the random generator rolls outside a chest, through
`generatedItemRecord` (`MM6ItemGenerator.h`): random ground treasure, `GiveItem` rolls, corpse loot and shop stock.
MM6.exe's generator (0x448790) zeroes the 28 bytes and writes only the item number, both bonuses, the charges and the
condition. A `GiveItem` with a fixed item puts its number over the rolled one, and a tier-7 ground treasure artifact
(0x44A6B0) gets nothing but its number. Items a map's delta puts on the ground take theirs from bytes 36 to 63 of their
100-byte ground record (`reconstructMM6GroundItems` in `MM6OutdoorDeltaSnapshot.h`), outdoors and indoors.

### Generation

`mm6::generateItem` (`MM6ItemGenerator.h`) is the original random item generator. It draws from the MM6 gameplay LCG,
the MSVC `rand` recurrence `state * 214013 + 2531011` returning bits 16 to 30 (`nextChestRandom15`). That state and
the generated-artifact flags live in `mm6::ChestTitleState` and are saved in `CHST`. Chests, shop stock, event item
gifts, monster loot and random ground treasure all draw from it. MM7's `ItemTable::generateItem` must not roll MM6
loot, its artifact branch gives MM7's ids 500 to 528, which are message scrolls in MM6.

- A request for any item first draws an artifact candidate at every treasure level. At level 6 it becomes that
  artifact with a 5% chance, when it wasn't generated before and fewer than 13 artifacts were. With the All Artifacts
  mod it only needs not to be collected, see [Mods and the Atlas](mods-and-atlas.md#all-artifacts-for-mm6).
- Otherwise the base item is a weighted pick over items 1 to 399 by treasure level. A roll of exactly 0 picks item 1,
  whatever its weight, as the original does.
- A request for a kind (20 to 43) filters by the original's type or skill codes. Other kinds are an item type plus 1.
  MM6.exe rolls `rand() % total` over the kind's candidates and takes the first one whose cumulative weight reaches
  it, so the first candidate gets one chance more than its weight, even at weight 0, and the last one one less. With
  `fix_item_generation`, on by default, the roll is one higher, as the GrayFace patch has it, and every candidate gets
  its weight. The any-item roll keeps MM6.exe's compare either way.
- A looted corpse's random item (MM6.exe 0x4218E0) asks for the monster's `monsters.txt` treasure level and kind
  (`mm6::generateTitleItem`), so the Great Wyrm, the Red, Blue and Gold Dragons, the Efreet and the Supreme Titan can
  drop artifacts. The gold dice, the drop chance and the 10% chance that the corpse stays to be looted again still
  draw from the shared RNG.
- Random ground treasure (MM6.exe 0x455ED0, `mm6::spawnRandomTreasure`) draws everything from the LCG: 20% nothing,
  40% gold and 40% an item of a random kind 20 to 43, both at the marker's own level. The map remap only decides
  whether the marker reaches tier 7, which goes to the direct artifact picker the chests use. No shipped marker is
  level 7, all 67 maps checked, and kinds 20 to 43 never roll an artifact, so ground treasure gives none in practice.
- The finalizer can give a weapon a special bonus, and armor to amulets a standard or a special bonus, with chances
  and strengths by treasure level from `stditems.txt` and `spcitems.txt`. MM6's second equipment chance row is a
  separate chance, not MM7's cumulative one.

### Standard and special bonuses

The record's standard bonus word is one-based into 14 MM6 attributes: the seven stats, HP, SP, AC and the Fire,
Electricity, Cold and Poison resistances (`kStandardBonusAttributes` in `MM6StandardBonusMap.h`). The shared runtime
stores Electricity, Cold and Poison as Air, Water and Body. Bonuses 1 to 13 have the same numbers as the shared
`Item_MM7` word, but its 14 is Earth. A chest record goes to and from a live chest only through `mm6::reconstructChest`
and `mm6::snapshotChest` (`MM6ChestRuntimeProjection.h`), which swap 14 and Body's 16 on armor to amulets.

Special bonuses are one-based ids into `spcitems.txt`. Only id 58, Antique, is modelled end to end. It multiplies the
item's value by 10 and does nothing else. The other specials appear on generated items and save with them
(`MM6SpecialItemProjection.h`), but their effects are open, except where a lane traced one (special 17, Recovery,
above).

### Artifacts and relics

Items 400 to 414 are MM6's 15 artifacts and 415 to 429 its 15 relics. Their bonuses aren't in the item records,
MM6.exe hardcodes them. Its item bonus routine (0x482E80) skips the standard and special bonus for any item above 399
and switches on items 401 to 429 instead. Other effects sit in the routines that need them. The port's table is
`mm6::kArtifactAttributeBonuses` (`MM6ArtifactBonus.h`), which `Character::GetItemsBonus` reads for MM6 in place of
MM7's artifact map, so the stats tab, hit points, spell points, armor class and resistances all see it. "Resistances"
are the four that items raise, Fire, Electricity, Cold and Poison, not Magic. A broken item gives nothing.

| Id | Name | Attribute bonuses (0x482E80) | Other effects in MM6.exe | Port |
|---|---|---|---|---|
| 400 | Mordred | none | Melee hit heals the wielder by a fifth of the hit while the monster still stands (0x430FEB) | Done |
| 401 | Thor | Might +20 | none, its "Force" isn't in the executable | Done |
| 402 | Conan | Might +20 | Doubles its damage roll against monsters 25 to 42, the devils, lizards, drakes, wyrms and dragons (0x47E4B0, 0x47E5CE) | Done |
| 403 | Excalibur | Might +30 | none | Done |
| 404 | Merlin | Spell points +40 | Swift, recovery -20 (0x481E61) | Done |
| 405 | Percival | none | Swift (0x481E69), carnage arrows (0x45CD5D) | Done |
| 406 | Galahad | Hit points +25, resistances +10 | none | Done |
| 407 | Pellinore | Endurance +30 | A hit point per regeneration step (0x487BB5) | Done |
| 408 | Valeria | Accuracy +30 | Halves arrows, but only from the armour slot (0x431F7E), where a shield can't go | Done, from either hand with `fix_valeria_shield` |
| 409 | Arthur | Seven stats +10, spell points +25 | none | Done |
| 410 | Pendragon | Luck +30 | Wards off a monster's poison from the cloak slot (0x481250, 0x481318, 0x4813C3), double disarm skill (0x485409) | Done |
| 411 | Lucius | Speed +30 | none | Done |
| 412 | Guinevere | Spell points +30 | Light and Dark spell skill x1.5 on a ring finger (0x422A9A, 0x422B12) | Done, no Light or Dark player spell is admitted yet |
| 413 | Igraine | Spell points +25 | Spirit, Mind and Body spell skill x1.5 on a ring finger (0x422932, 0x4229AA, 0x422A22) | Done |
| 414 | Morgan | Spell points +20 | Fire, Air, Water and Earth spell skill x1.5 as the amulet (0x422829) | Done |
| 415 | Hades | Luck +20 | Melee hit takes 20 more hit points (0x430FDA), disarm bonus (0x48543A), drains a hit point per regeneration step (0x487BC4) | Done |
| 416 | Ares | Fire resistance +25 | Melee hit takes 30 more hit points (0x430FC9) | Done |
| 417 | Poseidon | Might, Endurance, Accuracy +20, Speed -10, armor class -10 | none | Done |
| 418 | Cronos | Hit points +100, Luck -50 | none | Done |
| 419 | Hercules | Might +50, Endurance +20, Intellect -30 | none | Done |
| 420 | Artemis | Resistances -10 | Arrow hit takes 20 more hit points (0x431615) | Done |
| 421 | Apollo | Resistances +20, Luck +20, Endurance -30 | none | Done |
| 422 | Zeus | Hit points, spell points, Luck +50, Intellect -50 | none | Done |
| 423 | Aegis | Luck +20, Speed -20 | Wards off a monster's stone gaze in either hand (0x481632), halves arrows (0x431FAA) | Done |
| 424 | Odin | Resistances +50, Speed -40 | none | Done |
| 425 | Atlas | Might +100, Speed -40 | none | Done |
| 426 | Hermes | Speed +100, Accuracy -40 | none | Done |
| 427 | Aphrodite | Personality +100, Luck -40 | none | Done |
| 428 | Athena | Intellect +100, Might -40 | none | Done |
| 429 | Hera | Hit points, spell points, Luck +50, Personality -50 | none | Done |

The item texts mostly agree. Thor and Conan carry a Might bonus their texts don't mention, Thor's "Force" and
Pellinore's "Hit Recovery" have no code behind them, MM6.exe compares against neither item number outside the
bonus routine. Galahad's "Protection" is the resistance bonus. Swift
artifacts shorten recovery through `mm6::isSwiftArtifact`, next to the Swift enchantment in
`Character::GetAttackRecoveryTime`.

What the other addresses do, and where the port has it:

- **Hits** (`MM6ArtifactEffects.h`, called from `Actor::DamageMonsterFromParty`). The melee hit routine walks the off
  hand, then the main hand. Mordred (0x430FEB) adds a fifth of the hit, after the monster's resistance, to the
  wielder's hit points, capped at the maximum, when the monster still has hit points after the hit and any relic
  damage before it. Hades (0x430FDA) and Ares (0x430FC9) take 20 and 30 hit points off the monster, and the arrow hit
  routine (0x431615) does the same for 20 when the bow is Artemis. MM6.exe subtracts these straight from the monster's
  hit points. The GrayFace patch sends them through the monster damage routine (0x421DC0) as Poison, Fire and
  Electricity, and gives a stoned monster none, which `gameplay.fix_bonus_damage_resistance` (default on) does. The
  status line counts them in the damage.
- **Slaying** (`mm6::weaponSlays`, `Character::CalculateMeleeDmgToEnemyWithWeapon`). The melee damage routine doubles a
  weapon's dice and modifier, before the dagger triple, when the weapon is Conan and the monster is 25 to 42, or when it
  has Demon Slaying and the monster is 25 to 30, or Dragon Slaying and 31 to 42. Conan skips its enchantment check.
- **Regeneration** (`RegeneratePartyHealthMana`). The routine at 0x4879F0 runs once five game minutes have passed since
  its last run. Pellinore (0x487BB5) adds a hit point up to the maximum. Hades (0x487BC4) takes one, knocks the wielder
  out below 0, not at 0, and kills once hit points plus Endurance drop below 1 (`mm6::drainHadesWielder`). Neither works
  on a dead or eradicated wielder. The port steps once per five-minute mark passed, as for every other regeneration
  source.
- **Spell skill** (`mm6::artifactSpellSkillLevel`, called where a spellbook cast reads its skill). The cast routine
  (0x4227E1 on) adds half the skill for a school enchantment, then half of that result again for Morgan in the amulet
  slot, or Igraine or Guinevere on any ring finger. Only the level grows, the mastery, cost and recovery don't.
  MM6.exe uses the boosted level as it is, Heroism's power and duration too (0x426C21), so the spell rules accept
  levels up to `mm6::kMaxCastSkillLevel`, 141, which a skill of 63 reaches with both boosts.
- **Warding** (`mm6::receiveSpecialAttack`, `MM6MonsterAttack.h`). The monster special attack routine checks the cloak
  slot for Pendragon before it poisons (0x481250, 0x481318, 0x4813C3), and both hands for Aegis before it petrifies
  (0x481632). The checks jump on the item number before they test it for breakage, so a broken Pendragon or Aegis still
  wards. The jump on breakage goes the same way, so in MM6.exe any broken cloak wards off poison and any broken item
  in hand wards off stone. The port deliberately differs, by the owner's decision of 2026-11-21: a broken item wards
  off nothing, so only an unbroken Pendragon or Aegis wards.
- **Shielding** (`mm6::arrowHalvings`, `MM6MonsterAttack.h`). The routine for an object that hits a character halves an
  arrow once for an unbroken Aegis in either hand (0x431FAA), and once for an unbroken Valeria in the armour slot
  (0x431F7E). A shield can't go there, so Valeria never halves anything in MM6.exe. With `gameplay.fix_valeria_shield`,
  default on and beyond GrayFace, an unbroken Valeria halves an arrow from either hand instead. Off, the port keeps the
  armour slot check.

### Shops and value

Selling an item with value 0, the blasters, is refused with `gameplay.fix_shops_buying_blasters`. Prices and stock are
on [Houses and services](mm6-houses-and-services.md).

## Potions and herbs

Drinking or eating is `mm6::drinkPotion` (`MM6Potion.h`), the original's use-item dispatch. MM6 potions have no power,
every amount is fixed. Herbs heal HP or SP a little or poison. Bottles heal, restore SP, set stat, AC or resistance
bonuses, cure conditions, apply a six-hour buff (Heroism, Haste, Stone Skin, Bless at power 5), or change age.
Essences raise one stat by 15 once per character and lower one or more others. Rejuvenation lowers every stat by 1 and
resets the age modifier.

- Stat and resistance potions assign their bonus, they don't add. A weaker potion after a stronger one lowers it.
- No empty bottle comes back.
- A repeated Essence is used up for nothing in the original. `gameplay.fix_wasted_black_potion` keeps it in the hand
  with an error sound.
- The empty bottle 163 and bottles 189 to 196 can't be drunk.

Mixing is refused. `useitems.txt` is loaded and validated into `mm6::PotionCombinations` (`MM6PotionCombinations.h`),
but the item-on-item path in `UIPopup.cpp` refuses every herb, bottle and scroll id until the mixing effects are
traced. Besides item results, the table has five kinds of non-numeric marker whose meaning is unknown.

## Spells

### Schools, ids and items

Spell ids are 1 to 99, eleven per school in skill order: Fire 1 to 11, Air 12 to 22, Water 23 to 33, Earth 34 to 44,
Spirit 45 to 55, Mind 56 to 66, Body 67 to 77, Light 78 to 88, Dark 89 to 99. The school skill is `12 + (id - 1) / 11`.
MM6 reuses the shared `SpellId` numbers, which name MM7 spells. `MM6PlayerSpell.h` gives MM6 names to the ids where
the shared name is wrong.

### Cost and recovery

Spell points and recovery by mastery come through `mm6::spellManaRecovery`. Recovery is the executable's spell table.
The cost is `spells.txt`'s while `gameplay.fix_spells_cost` is on, as the GrayFace patch has it, and the executable's
otherwise. They differ only for Static Charge at Expert, 1 against 2. The cast path, the spellbook popup and the
quick-spell key all use it. A cast with an overridden skill (scroll, NPC, temple) or
under the debug all-magic switch throws, its cost rule isn't verified.

### What can be cast

`mm6::isVerifiedPlayerSpell` (`MM6SpellEffectGuard.h`) is the list of spells the player can cast from the spellbook.
The interface turns down any other regular spell (`shouldRefusePlayerSpell`), and the cast queue throws on one.

| Kind | Spells (ids) |
|---|---|
| Attack | Flame Arrow 2, Fire Bolt 4, Static Charge 13, Sparks 15, Lightning Bolt 18, Ice Bolt 28, Magic Arrow 35, Spirit Arrow 45 |
| Travel | Feather Fall 16, Jump 19, Fly 21, Water Walk 27 |
| Support | Torch Light 1, Fire 3, Wizard Eye 12, Electricity 14, Cold 25, Poison 69 and Magic 36 protections, Bless 46, Healing Touch 47, Heroism 51 |
| Healing | Cure Weakness 67, First Aid 68, Cure Wounds 71 |

Attack spell damage is `mm6::playerSpellDamage`. Flame Arrow, Magic Arrow and Spirit Arrow roll to hit against armor
class like arrows, the others always hit. Monster resistance uses MM6's own columns. Spells cast by events and by
monsters have their own verified lists (`isVerifiedEventSpell`, `isVerifiedMonsterSpell`), covered on
[Events](mm6-events.md) and [Combat and monsters](mm6-combat-and-monsters.md).

Not castable yet, among others: Shield, Stun, Town Portal, Lloyd's Beacon, the other condition cures. Power Cure's heal
rule is in source (`MM6PowerCureRules.cpp`) but the spell isn't on the list.

Scrolls and spellbook items are not implemented. Using a scroll throws in `Character::useItem` before the item is
consumed. Reading a spellbook refuses a character whose class can't learn the school or who lacks the school skill,
and throws otherwise. Spell guilds stock spellbooks as items, so a spell can't yet be learned in play. Game tests
teach spells in code.

### Durations and cure windows

- Fly, Water Walk and Feather Fall last 5 minutes per skill point at Normal, 10 at Expert, an hour at Master
  (`travelBuffDuration`).
- Bless, Heroism and Shield last 64 minutes plus 5 minutes per point, 15 at Master (`blessAndHeroismDuration`).
  Heroism's power is skill + 5.
- Healing Touch heals 2d3 plus 1, 3 or 5 by mastery. First Aid heals 5, 7 or 10.
- Water Walk's mana drain follows the shared upstream flag `gameplay.fix_water_walk_mana_drain`, every 20 minutes
  with it and every 5 without.
- The six condition cures accept a condition up to a skill-scaled age, see
  [Rules from MM6.exe](mm6-executable-rules.md#rules-kept-as-source-constants) and `gameplay.fix_condition_spells`.

### Buffs

MM6 buffs use the shared `PartyBuff` and `CharacterBuff` slots, not MM6's own slot numbers. Every writer, the HUD,
the stat getters and the save's `BUFF` section agree on that. A monster's Dispel Magic resets every shared slot, as the
original clears every buff it has. Arrow halving reads the shared Shield slot.

`SpellBuff::Apply` refuses a buff that would end sooner than the one in place, as the original does.
`gameplay.fix_stronger_shorter_buff` lets a strictly stronger buff replace it anyway. Power alone is compared.
`gameplay.fix_buff_duration_text` shows the two largest units of the remaining time.

## Awards

`awards.txt` has 100 numbered rows and no priority column values. `AwardTable.cpp` loads them for MM6 with all
priorities at 0, refusing a missing or duplicate row. Rows whose text holds a `%` counter (bounties, deaths, prison
terms and arena wins) are not shown, their MM6 counters aren't mapped. Guild membership is award `64 + guild`
(`MM6House.cpp`), and the Knight promotion awards gate some teacher prices.

## Source

- [MM6CreationRules.h](../src/Engine/MM6/MM6CreationRules.h), [MM6CreationVitals.h](../src/Engine/MM6/MM6CreationVitals.h),
  [MM6CreationItemSeed.h](../src/Engine/MM6/MM6CreationItemSeed.h), [MM6Calendar.h](../src/Engine/MM6/MM6Calendar.h)
- [Character.cpp](../src/Engine/Objects/Character.cpp) MM6 branches of creation, HP, SP and item use
- [MM6ActualStatCondition.h](../src/Engine/MM6/MM6ActualStatCondition.h), [MM6NewDay.h](../src/Engine/MM6/MM6NewDay.h)
- [MM6ItemGenerator.h](../src/Engine/MM6/MM6ItemGenerator.h), [MM6ChestBaseItemPicker.h](../src/Engine/MM6/MM6ChestBaseItemPicker.h),
  [MM6ItemRecord28.h](../src/Engine/MM6/MM6ItemRecord28.h), [MM6StandardBonusMap.h](../src/Engine/MM6/MM6StandardBonusMap.h),
  [MM6AntiqueItemProjection.h](../src/Engine/MM6/MM6AntiqueItemProjection.h), [MM6BoundedItemProjection.h](../src/Engine/MM6/MM6BoundedItemProjection.h),
  [MM6ArtifactBonus.h](../src/Engine/MM6/MM6ArtifactBonus.h)
- [MM6Potion.h](../src/Engine/MM6/MM6Potion.h), [MM6ConsumableRules.h](../src/Engine/MM6/MM6ConsumableRules.h),
  [MM6PotionCombinations.h](../src/Engine/Tables/MM6PotionCombinations.h)
- [MM6PlayerSpell.h](../src/Engine/MM6/MM6PlayerSpell.h), [MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h),
  [MM6SpellLearning.h](../src/Engine/MM6/MM6SpellLearning.h), [MM6SpellItems.h](../src/Engine/MM6/MM6SpellItems.h)
- [MM6SpellRules.h](../src/Engine/Spells/MM6SpellRules.h), [MM6TimedCureRules.h](../src/Engine/Spells/MM6TimedCureRules.h),
  [MM6FirstAidRules.h](../src/Engine/Spells/MM6FirstAidRules.h), [MM6PowerCureRules.h](../src/Engine/Spells/MM6PowerCureRules.h),
  [MM6CombatBuffRules.h](../src/Engine/Spells/MM6CombatBuffRules.h), [CastSpellInfo.cpp](../src/Engine/Spells/CastSpellInfo.cpp)
- [AwardTable.cpp](../src/Engine/Tables/AwardTable.cpp)
- [GameConfig.h](../src/Application/GameConfig.h) for the `gameplay.fix_*` flags
- Unit tests in [src/Engine/MM6/Tests](../src/Engine/MM6/Tests) and [src/Engine/Spells/Tests](../src/Engine/Spells/Tests).
  Game tests `MM6PlayerSpells.*`, `MM6Potion.*`, `MM6ArtifactBonus.*` and the other `GameTests_MM6*.cpp` files in
  [test/Bin/GameTest](../test/Bin/GameTest)

See also: [Rules from MM6.exe](mm6-executable-rules.md), [MM6 save format](mm6-save-format.md),
[MM6 houses and services](mm6-houses-and-services.md), [MM6 combat and monsters](mm6-combat-and-monsters.md),
[MM6 events](mm6-events.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md), [Open questions](open-questions.md)
