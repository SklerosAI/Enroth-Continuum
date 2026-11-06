# MM6 class, skill, and party creation audit

This is an implementation specification for the isolated MM6 runtime. It records identifiers and behavior, not game-owned descriptions. MM7 and MM8 retain their own character tables and creation flows.

## Evidence and confidence

- **Retained MM6 data:** private `local-game-data/mm6/research-2026-11-05/icons/class.txt` has one header and 18 class rows; `skilldes.txt` has one header and 31 skill rows. Row numbers below are zero based after the header. These counts and names were checked directly on 2026-11-05.
- **Owned documentation:** private `mm6-manual.txt` (character classes, pages 24–25) and `mm6-reference-card.txt` (Create Party, Quick Start, four characters, starting skills) agree with the data's six starting class families and the four-skill creation rule.
- **Independent ID cross-check:** [MMExtension's `const.Class` reference](https://grayface.github.io/mm/ext/ref/) explicitly identifies MM6 `Cavalier=1`, `HighPriest=5`, and `BattleMage=13`, supporting the class table's zero-based sequence. Treat the full native class ID sequence as highly likely, but validate against MM6 save/event behavior before committing a binary format.
- **Inherited code:** `src/Engine/Objects/CharacterEnums.h`, `src/Engine/Objects/CharacterEnumFunctions.h`, `src/Engine/Localization.cpp`, `src/Engine/mm7_data.cpp`, `src/Engine/Objects/Character.cpp`, and `src/GUI/UI/UIPartyCreation.cpp` are the current MM7-oriented implementation examined here.

## Class IDs and promotions

The table order provides the intended MM6 IDs for implementation. Each row is a promotion family; only the first class is selectable at party creation.

| MM6 IDs | Starting class → first → second promotion | Inherited enum conflict |
| --- | --- | --- |
| 0–2 | Knight → Cavalier → Champion | IDs 0–2 happen to agree. |
| 3–5 | Cleric → Priest → High Priest | MM7 ID 3 is Black Knight; MM7 Cleric starts at 24. |
| 6–8 | Sorcerer → Wizard → Arch Mage | MM7 IDs 6–8 are Spy, Assassin, Monk; Sorcerer starts at 32. |
| 9–11 | Paladin → Crusader → Hero | MM7 IDs 9–11 are Initiate, Master, Ninja; Paladin starts at 12. |
| 12–14 | Archer → Battle Mage → Warrior Mage | MM7 ID 12 is Paladin; MM7 Archer starts at 16, with a different promotion name and order. |
| 15–17 | Druid → Great Druid → Arch Druid | MM7 ID 15 is Villain; Druid starts at 28. |

MM6 has six families and three tiers; inherited `getTier*Class` helpers use a four-ID family mask and light/dark final branches. MM6 promotion lookup therefore requires its own table, not `& ~3`. Class-dependent HP, SP, stat bonuses, spell access, skill availability and mastery ceilings also require MM6 tables. `Character::GetMaxHealth`, `GetMaxMana`, `ChangeClass`, and `GetSkillIdxByOrder` currently consume MM7 class IDs and affinity data. `ChangeClass` and `GetSkillIdxByOrder` divide class ID by four to index nine MM7 families; applying MM6 ID 3 would select Knight affinity for Cleric. The manual documents promotion-related per-level gains, but exact starting values and formulae need verification from MM6 data or observed game behavior before implementation.

## Skill ID map

`skilldes.txt` has exactly five tab-separated fields per row: name, description, Normal, Expert, Master. It has no Grandmaster column. MM6 skill row order is:

| MM6 IDs | Skills |
| --- | --- |
| 0–7 | Staff, Sword, Dagger, Axe, Spear, Bow, Mace, Blaster |
| 8–11 | Shield, Leather, Chain, Plate |
| 12–20 | Fire, Air, Water, Earth, Spirit, Mind, Body, Light, Dark |
| 21–29 | Identify, Merchant, Repair, Bodybuilding, Meditation, Perception, Diplomacy, Thievery, Disarm Traps |
| 30 | Learning |

The inherited `Skill` enum agrees through ID 29 (allowing display-name differences such as `SKILL_ITEM_ID` and `SKILL_TRAP_DISARM`). At ID 30 it has `SKILL_DODGE`; inherited `SKILL_LEARNING` is ID 36. IDs 30–35 are inherited MM7-only skills: Dodge, Unarmed, Monster ID, Armsmaster, Stealing, Alchemy. The enum also has hidden Club 37 and Misc 38; their MM6 representation and equipment behavior must be determined separately. `Localization::initializeSkillDescriptions` zips MM6 rows to all 37 inherited visible skills and expects six columns, so MM6 Learning text would be attached to Dodge and its Master/Grandmaster fields need a format-specific parser. `initializeClassNames` similarly zips 18 MM6 rows onto MM7 class slots, starting the Cleric description at Black Knight. Use explicit MM6 ID-to-runtime mappings and title-specific parsers; do not rely on shared row ordinals.

MM6 has Normal, Expert and Master proficiency in the retained table. Disable Grandmaster training, UI controls, and effects for MM6. `Thievery` occurs in the table but is described there as unused; its visibility and mechanics remain an open validation item. `Diplomacy` is present despite being annotated unused for MM7 in the inherited enum. Retain the MM6 IDs and establish actual trainability from MM6 skill tables or a reference playthrough.

## Creation screen and starting state

The MM6 reference card specifies four party members. Each character starts with two class-determined skills and chooses two more; all four must be present to continue. Changing class changes starting statistics and skills. The inherited `PlayerCreation_Choose4Skills` count matches the total, but its available-skill set comes from MM7 `pSkillAvailabilityPerClass`, so the selections are wrong for MM6. Build a six-row MM6 affinity table with exact two primary skills, eligible optional skills, and per-class mastery ceilings, then verify every base class in the creation UI. Do not infer the table from MM7 affinities.

`UIPartyCreation.cpp` renders nine MM7 base classes (including Thief, Monk, Ranger) and loads nine class icons. The retained MM6 icon index has `ic_knig`, `ic_cler`, `ic_sorc`, `ic_palad`, `ic_arch`, and `ic_druid`; the requested MM7 `IC_KNIGHT`, `IC_THIEF`, `IC_MONK`, and `IC_RANGER` names are absent. The screen also constructs 22 MM7 portrait names from `pPlayerPortraitsNames`; MM6's retained icon names instead include `malea*`, `maled*`, and `girla*` families. Inventory all MM6 portrait frames, face and voice IDs, sex associations, animations, and UI chrome before wiring the screen. Current face and voice cycling assumes 20 IDs and `SetSexByVoice` has a fixed MM7 mapping, so both need MM6-specific validation.

The MM6 manual and reference card establish the creation rules, but do not settle every runtime constant. Verify default Quick Start party, starting HP/SP and base attributes, initial equipment, class-specific skill choices, portrait/voice pairings, training caps, and the two promotion quest transitions against original MM6 behavior. Preserve save/load IDs when translating to any internal shared enum.

## Additional inherited consumers found in F2

This source review used the retained private tables and the current C++ tree;
it did not launch MM6 or establish the exact starting constants. `class.txt` and
`stats.txt` supply descriptions, not a machine-readable starting-party or
numeric stat matrix. The six-class starting matrix must be derived and checked
against the owned documentation and original behavior before F4.

| Current source path | Inherited assumption | MM6 acceptance obligation |
| --- | --- | --- |
| `Party.cpp` (`Reset`, `createDefaultParty`) and `UIPartyCreation.cpp` (completion) | Reset/default party seed MM7 classes, faces, skills, inventory and opening quest/calendar state. The custom creation screen always takes the MM7 reset path. | Define distinct MM6 Create Party and Quick Start initial states. For each, prove four legal characters, starting skills/items/spells, flags, time and New Sorpigal position, then save/reload. No MM7-only Thief, skill or quest bit may leak in. |
| `Character.cpp` (`GetMaxHealth`, `GetMaxMana`, `ChangeClass`, race/face helpers) and `CharacterEnumFunctions.h` | MM7 race/face bands, four-ID class families, light/dark tiers and MM7 HP/SP tables. A class change currently changes skills/HP/SP without the MM6 starting-stat adjustment described by the reference card. | Reconcile MM6 class-specific starting attributes, 50-point allocation bounds, per-level HP/SP, both promotion tiers and retroactive gains. A portrait must not silently select an MM7 race rule. |
| `mm7_data.cpp`, `GUIWindow.cpp`, `UICharacter.cpp`, `NPCTopics.cpp`, `UIHouses.cpp` and `MagicGuild.cpp` | Nine-family affinity/cap tables, MM7-only skills, Grandmaster teacher/topic arithmetic and UI grouping. | Show and train only legal MM6 skills through Master; test all six classes, two fixed plus two chosen creation skills, Expert/Master teaching and promotion messaging, and Learning at MM6 ID 30. Resolve whether the listed but unused Thievery skill is trainable. |
| `MM6ItemTable.cpp`, `ItemEnumFunctions.h`, `ItemTable.cpp`, `UIPopup.cpp` and `Character.cpp` | MM6 item rows still flow into shared MM7 kind/bonus logic; potion mixing uses MM7 potion IDs, its matrix and an Alchemy skill absent from MM6's 31-skill table. | Classify every MM6 item ID before use. Reconcile reagent and potion combinations, successes, failures, side effects and bottle return without an MM7 Alchemy gate. Exercise equipment, scrolls, books, wands and artifacts through use and save/reload. |
| `Character.cpp` and `ItemEnums.h` | MM7 resistance taxonomy and Grandmaster Learning arithmetic can affect MM6 spell, damage and experience rules. | Check each MM6 damage/resistance family and Learning at Normal, Expert and Master. Spell ID and effect mapping remains a separate full 99-spell obligation in the implementation checklist. |

Promotion **triggers and rewards in EVT/NPC dialogue** belong to the event and
campaign audit; **character ID and flag serialization** belongs to the save
audit. Both must agree with the class and skill tables before the consolidated
F3 preflight. Exact potion recipes and numeric formulas remain open F2
questions, even though the current code collisions are confirmed.

### Creation and alchemy rule confidence

The owned MM6 reference card and manual establish a shared 50-point creation
pool that must be spent, four characters, six base classes, and two fixed plus
two class-eligible optional skills per character. They describe lowering an
attribute no farther than two below its class base and raising it no higher
than 25 in one-point steps; changing class resets prior stat adjustments and
starting skills. The Prima guide suggests class-dependent base vectors, but
the exact six vectors, fixed/optional skill matrix, starting HP/SP, gold,
food, letter, equipment, spellbooks and Quick Start characters still require
reconciliation. A guide's apparent Quick Start composition is not enough to
freeze the shipped default state.

`Character.cpp` currently uses an MM7 face/race stat table and a shared pool;
`Party.cpp` and `UIPartyCreation.cpp` seed an MM7 default party and MM7 opening
state. Test all six MM6 base classes with the exact pool boundary, base minus
two accepted/base minus three rejected, 25 accepted/26 rejected, class change
after edits, incomplete skill selection, Quick Start, and save/reload. Keep
the official numeric rule distinct from the still-unverified class constants.

The retained MM6 item classification places herbs and bottles in an ID band
outside the current `isReagent`/`isPotion` ranges in
`ItemEnumFunctions.h`. `UIPopup.cpp` therefore cannot reach its MM7 potion
mixing branch for those MM6 IDs; it also gates results on `SKILL_ALCHEMY`,
which MM6's skill table and the
[MMExtension skill reference](https://grayface.github.io/mm/ext/ref/) do not
include for MM6. `MM6ItemTable.cpp` loads no MM6 potion-combination matrix.
The retained `useitems.txt` has a complete symmetric 29 × 29 matrix whose
header and row axes are matching MM6 herb/bottle item IDs; see the
[table-consumer audit](MM6_TABLE_CONSUMER_SPEC.md). Its result-token meanings
and failure effects are still unverified.
The manual/reference card confirm herb-to-bottle and potion-to-potion
interaction and hazards; the Prima and NWC guides provide provisional recipe
and side-effect leads, not an audited complete matrix. Reconcile exact
ingredient/result IDs, order symmetry, failed mixes, power scaling, bottle
return, black-potion one-time effects and rejuvenation before F4. Test those
paths without an Alchemy skill and through save/reload.

## Implementation gates

1. Parse MM6 `class.txt` and `skilldes.txt` as their actual two-field and five-field formats, assert expected counts and named row IDs, and reject mismatched data cleanly.
2. Supply MM6 class/skill ID mappings, six affinity rows, three-tier promotion lookup, HP/SP and attribute tables, and mastery caps. Keep these adjacent to, not substituted for, MM7 and MM8 tables.
3. Supply MM6 creation layout and native icon/portrait/voice indices. Create and Quick Start must lead to a valid four-character party.
4. For each of the six starting classes, confirm two fixed skills plus two legal selections, class-change reset, starting health/mana, and refusal to continue with fewer than four skills or unspent required attribute points.
5. Exercise first and second promotions for all six families, training through Master, save/reload of class and skill IDs, and at least one MM7 regression launch. Compare observed screenshots and UI interaction as well as serialized state.

No runtime code was changed by this audit.
