# MM6 rules lane: source joins and F2 closure obligations

Status: **F2 evidence, not an implemented rule set** (2026-11-05). This lane
compares the retained ignored MM6 tables, the owner's scanned guide, a primary
implementation reference, and current C++ consumers. It did not open original
archives again or launch the partial MM6 runtime. Exact table rows, recipe
triples, guide transcription, and per-ID results stay in ignored local reports.

## Potion matrix and guide disagreement

The retained `useitems.txt` matrix has 25 unordered pairs with a numeric
result. A manual visual transcription of the safe-mixture chart in the
[Prima strategy guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) on
printed pp. 230–231 (PDF pp. 238–239) has exactly the same 25 unordered input
pairs. Twenty-three result rows agree; **two herb-and-bottle results disagree**
with the retained table. `scripts/mm6_probe_potion_guide.py` reproduces that
comparison using the ignored transcription and
`unloaded-table-probe.json`, writing exact differing rows and input hashes only
to ignored `potion-guide-reconciliation.json`. The scan itself was checked at
both pages, so this is not merely an OCR difference. The shipped table is the
stronger source for the installed GOG data, but F3 must still check the
original table and original-game action before implementing the two disputed
results.

The guide describes the listed pairs as safe and other mixtures as potentially
destructive. The retained matrix has five other result-token classes by exact
value: four letter-digit marker codes and one two-letter marker code. Their
specific meanings, damage, bottle return and side effects are **unverified**;
the public probe reports only their lexical classes. Numeric results likewise
do not prove the original runtime's inventory or potion-power behavior.

The current [MM6 item loader](../src/Engine/Tables/MM6ItemTable.cpp) never
loads `useitems.txt`. The [item kind predicates](../src/Engine/Objects/ItemEnumFunctions.h)
and [potion interaction](../src/GUI/UI/UIPopup.cpp) use MM7 ranges and an
Alchemy skill gate. MM6's retained skill table has no Alchemy row, so the
current branch cannot express these MM6 combinations through that inherited
path. A title-specific MM6 item action should use the matrix's actual item
IDs, retain a separate MM7 branch, and reject unknown active tokens rather
than silently treating them as success.

## Starting character data source

The owner's Prima guide (printed pp. 16–22, PDF pp. 24–30) describes a
seven-stat base distribution per class of two 14s, two 11s, one 9 and two 7s,
plus the creation bounds already captured in
[the class audit](MM6_CLASS_SKILL_SPEC.md). This gives a **shape and range
check**, not the seven-value ordering for each of six classes. The primary
[MMExtension reference](https://grayface.github.io/mm/ext/ref/) exposes the
MM6 `GameClassKinds.StartingStats[class][stat]` and
`StartingSkills[class][skill]` arrays. Its documented MM6 skill codes mean
unavailable, initially granted, selectable at creation, and learnable later
(0, 1, 2, 3 respectively); MM7+ uses a different coding. This is a concrete
F3 extraction target for the original game or a trusted local runtime
reference. It does **not** supply the array values, Quick Start defaults,
starting equipment, spellbooks or promotion caps by itself.

## Consumer-to-acceptance matrix

| Rule family | Retained/guide evidence | Current consumer and confirmed gap | Required F2/F3 resolution and F4/F5 acceptance |
| --- | --- | --- | --- |
| Six class bases and two promotion tiers | `class.txt`, manual and Prima class pages, MMExtension starting-array schema | [Character::ChangeClass and GetSkillIdxByOrder](../src/Engine/Objects/Character.cpp) index four-ID MM7 families. | Obtain exact six base stat/skill rows, HP/SP factors, promotion IDs and retroactive gains. Create each class at the bounds, promote twice, and reload without an MM7 class alias. |
| Creation and Quick Start | Manual/reference card: four characters, two fixed and two chosen skills; guide gives class distribution shape | [Party::Reset/createDefaultParty](../src/Engine/Party.cpp) seeds MM7 classes, faces, items and quest state; [UIPartyCreation](../src/GUI/UI/UIPartyCreation.cpp) shows MM7 choices. | Reconcile exact shipped defaults and legal optional skills. Test custom and Quick Start, class switch after stat changes, incomplete selection rejection, initial map and save/reload. |
| Skill IDs and proficiency | `skilldes.txt`: MM6 IDs 0–30, Normal/Expert/Master | [Localization](../src/Engine/Localization.cpp) and [MM7 data](../src/Engine/mm7_data.cpp) zip/reuse different IDs and Grandmaster caps. | Assign explicit MM6 ID mapping, teacher requirements and class ceilings; test Learning ID 30, all six class eligibility sets, Expert/Master, and no Grandmaster UI/effects. |
| Equipment and item categories | `items.txt`, `rnditems.txt`, `stditems.txt`, `spcitems.txt`, `scroll.txt` | [MM6ItemTable](../src/Engine/Tables/MM6ItemTable.cpp) loads data, but [ItemEnumFunctions](../src/Engine/Objects/ItemEnumFunctions.h) and [ItemTable](../src/Engine/Tables/ItemTable.cpp) retain MM7 category/treasure logic. | Classify every active ID and enchantment, spell scroll/book/wand and artifact behavior; test equip restrictions, generated treasure, identify/repair, use, drop and save/reload. |
| Potion combinations | `useitems.txt` plus guide chart; 25 safe input pairs, two guide-result disagreements | No MM6 matrix loader; [UIPopup](../src/GUI/UI/UIPopup.cpp) uses MM7 potion IDs and Alchemy. | Decode every matrix token and failure effect, settle the two disputes at F3, exercise all 841 cells offline and representative success/failure/black-potion cases live. |
| 99 spells and nine schools | `spells.txt`, `scroll.txt`, manual/guide spell sections; per-ID private reconciliation | [SpellStats](../src/Engine/Spells/Spells.cpp) and effect dispatch use inherited IDs/cost/resistance assumptions; 11 damage labels fall to physical. | Fix IDs and all three mastery effects, costs, target/duration/resistance, scroll/book/monster casts; test one or more live spells in each school and full offline ID coverage. |
| Monster attacks and identity | `monsters.txt`, `dmonlist.bin`, private parser report | [Monsters](../src/Engine/Objects/Monsters.cpp) rearranges MM6 rows into shared parser; [MonsterEnumFunctions](../src/Engine/Objects/MonsterEnumFunctions.cpp), [Actor](../src/Engine/Objects/Actor.cpp), and hostility keep MM7 assumptions. | Eliminate silent fallback for active attack, missile, spell and special labels; map families, drops, resistances and factions; prove melee/ranged/caster/status/neutral cases, loot and persistence. |

The spell and monster aggregate mismatch counts and detailed source locators
remain in [the implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md),
[monster audit](MM6_MONSTER_COMBAT_SPEC.md), and ignored reports. A matching
name or parsed row is not a semantic or runtime check.

## Next decisions for the integrator

1. Add the private `prima-potion-recipes-private.json` and
   `potion-guide-reconciliation.json` to the private report registry; the
   tracked script and this document are safe to commit, but the input and
   output must remain ignored.
2. Freeze an F3 extraction/check list for the original MM6 class arrays,
   potion marker effects, 25 numeric results, and all current parser/default
   collisions. Do not treat the guide's two disputed results as the game rule.
3. Continue the full item/spell/monster row-to-consumer audit. This document
   closes **no** F2 gate; active effects, starting constants and combat
   semantics are still unresolved.

See [MM6 guide reconciliation](MM6_GUIDE_RECONCILIATION.md),
[table consumer audit](MM6_TABLE_CONSUMER_SPEC.md),
[class/skill audit](MM6_CLASS_SKILL_SPEC.md), and
[MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md).
