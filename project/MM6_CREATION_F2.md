# MM6 party creation and progression: F2 source join

Status: **audit and acceptance specification, not implementation** (2026-11-05).
This compares the owner's retained manual, reference card and Prima guide with
the current C++ source and the already retained MM6 item/spell indexes. It did
not reopen original MM6 archives or run the partial MM6 game. Exact guide
transcription, per-class arrays, item names and per-ID spell joins remain in
ignored local research, outside Git. See the broader
[class and skill specification](MM6_CLASS_SKILL_SPEC.md),
[rules closure](MM6_RULES_F2_CLOSURE.md) and
[F3 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md).

## Evidence and confidence

| Evidence | Locator | What it establishes |
| --- | --- | --- |
| Owned MM6 reference card | Ignored `mm6-reference-card.pdf` and extraction, opening Create Party/Quick Start instructions | Four characters; portraits, names, class, stats and skills editable; 50 shared points; each stat from class base minus two through 25 in one-point changes; two fixed plus two selected class-eligible skills per character; class change resets starting stats and skills. This is **documented behavior**, not the exact shipped starting arrays. |
| Owned MM6 manual | Ignored `mm6-manual.pdf`, character creation/class section around printed pp. 24–25 | Six base classes and the user-facing creation/progression model. Its prose does not supply a complete numeric initialization table. |
| [Prima MM6 strategy guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) | Printed pp. 16–22, PDF pp. 24–30 | Class descriptions and a seven-stat base **distribution shape** per class: two 14s, two 11s, one 9, two 7s. This does not establish each class's stat ordering, fixed/optional skills, or exact Quick Start state. |
| [MMExtension reference](https://grayface.github.io/mm/ext/ref/) | `GameClassKinds.StartingStats` and `StartingSkills` | Names a concrete extraction surface for MM6 class arrays. MM6 skill codes 0–3 distinguish unavailable, granted, selectable, and later learnable; this schema is not itself the values. |
| Current C++ and retained data indexes | Links below and ignored `creation-rules-probe.json` | Confirms inherited MM7 routing and literal ID collisions. It does **not** prove MM6's original inventory, spell behavior, or playability. |

## Creation and Quick Start routing

[Game.cpp](../src/Application/Game.cpp) calls `PartyCreationUI_Loop()` for a
new game and chooses the pending MM6 map afterward. In
[UIPartyCreation.cpp](../src/GUI/UI/UIPartyCreation.cpp), that loop calls
`Party::Reset()` and `createDefaultParty()` before presenting the inherited
screen. There is no MM6 Create Party versus Quick Start branch at this point.
The `GameStarterOptions.quickStart` flag in
[GameStarterOptions.h](../src/Application/Startup/GameStarterOptions.h) skips
slow application initialization; it is not the documented MM6 Quick Start.

[Party.cpp](../src/Engine/Party.cpp) currently seeds a Knight, **Thief**,
Cleric and Sorcerer with MM7 faces, starting skill state and opening party
state. `createDefaultParty()` additionally uses MM7-only Stealing and Alchemy
skills. The retained MM6 class/skill tables have no such starting class or
skill entries. The current UI exposes **nine** MM7 class buttons while MM6
has six selectable base classes; some buttons carry IDs that denote different
MM6 promoted classes. Selecting an inherited button cannot be treated as a
valid MM6 class change merely because the integer is in range.

The screen's class icon index divides class IDs by four, and its portrait/voice
handling comes from MM7 arrays and 20-ID cycling. The retained MM6 icon index
has its own six base class icons and face families. Portrait, sex, voice and
animation identity need an explicit MM6 mapping; a portrait must not select a
stat rule through an MM7 race derivation.

**Acceptance:** Create Party and Quick Start each produce four legal MM6
characters on the correct initial map with title-specific flags, date,
position, gold/food, skills, health/mana, inventory and known spells. The two
routes must save and reload to identical character and party state. The
Quick Start composition is an extraction target, not something to infer from
the guide's suggested party.

## Stats, class changes and skill selection

[Character.cpp](../src/Engine/Objects/Character.cpp) computes creation points
from `StatTable[GetRace()]`, derives race from the portrait, initializes
stats from that race table, and uses its variable step/bound values for both
attribute arrows. This conflicts with the documented MM6 class-based bases,
one-point adjustments and 25 ceiling. `ChangeClass()` sets class/skills and
recomputes health/mana, but does not restore the documented new class's base
stats after prior adjustments. Both `ChangeClass()` and `GetSkillIdxByOrder()`
use `classType / 4` into nine-family MM7 skill availability. The MM6 families
have three IDs each; MM6 Cleric ID 3 would index the inherited Knight family.

The retained MM6 `class.txt` and `skilldes.txt` establish 18 class rows and
31 skill rows, but not all six numeric starting vectors or legal selection
sets. `StartingStats` and `StartingSkills` must be extracted before F4, then
cross-checked against the owned reference card and a controlled original-game
creation sequence at F3. MM6 Learning is skill ID 30, whereas the inherited
enum places Dodge there and Learning at 36. Normal, Expert and Master are the
MM6 proficiency set; a Grandmaster option would be an inherited leak.

**Acceptance matrix:**

| Scenario | Required result |
| --- | --- |
| Each of six base classes | Exact class stat vector, two fixed skills and only the legal optional set; four skills total after two selections. |
| Point pool and bounds | Shared 50-point pool; class base minus two accepted, minus three rejected; 25 accepted, 26 rejected; one-point increments; continue rejected with unspent required points. |
| Change class after edits | New class base stats, legal skill choices and fixed skills restored; no stale optional skill or spent-point accounting. |
| Portrait/voice cycling | Each valid MM6 face, sex and voice pairing displays correctly and preserves the selected **class** stat base. |
| Save/reload | Base class, attributes, known skills and portrait/voice IDs persist without reinterpretation as MM7 IDs. |

## Starting items and spells: a confirmed literal collision

The inherited completion block in [UIPartyCreation.cpp](../src/GUI/UI/UIPartyCreation.cpp)
fills health/mana, adds a random level-2 ring, then grants equipment,
spellbooks, known-spell flags and a potion bottle according to inherited
skill predicates. Its literal constants are MM7 item and spell enums. The
retained [MM6 item index](MM6_TABLE_CONSUMER_SPEC.md) classifies MM6 spellbooks
in the 300–398 band. All **seven** inherited `ITEM_SPELLBOOK_*` literals in
this block fall outside that band and none resolves to an MM6 spellbook
category. The inherited bottle constant resolves to an MM6 spell-scroll
category. Seven inherited spell flags are set; two carry a different or absent
retained same-ID label. A matching label for the other five does not prove
matching spell effect. The existing ring and gear grants are also unverified
as original MM6 starting inventory.

Reproduce this aggregate without opening original archives:

```sh
python3 scripts/mm6_probe_creation_rules.py \
  local-game-data/mm6/research-2026-11-05 \
  local-game-data/mm6/research-2026-11-05/creation-rules-probe.json
```

The script checks retained input hashes, source literals and categories. Its
ignored JSON contains the exact per-ID joins and guide/source hashes; stdout
prints aggregate counts only. Numeric/category collisions establish that the
current startup grants cannot be used as an MM6 rule. They do not establish
what the correct starting item IDs should be.

**Acceptance:** For each legal base class and both new-game routes, compare
actual initial equipped and carried items, spellbooks, known spells and
non-item state to F3 evidence; identify every ID through an MM6-specific
lookup, then save/reload and exercise use/equip where applicable. No MM7-only
ring, bottle or spellbook grant may survive by accident.

## Progression and promotion consumers

The inherited [Character.cpp](../src/Engine/Objects/Character.cpp) HP/SP,
Learning and `ChangeClass()` paths and [mm7_data.cpp](../src/Engine/mm7_data.cpp)
availability/mastery tables are MM7 oriented. The class spec records MM6's
six three-tier promotion sequence; four-ID MM7 light/dark family arithmetic
cannot produce it. Training and character UI consumers must use MM6-specific
skill IDs and caps. Event/NPC promotion triggers and awards require a separate
world-state join; changing the class enum alone would not deliver the correct
quest reward or retroactive HP/SP effect.

**Acceptance:** For all six families, exercise base, first and second tiers;
check class ID, HP/SP per-level values, retroactive changes, legal training
through Master, displayed promotion text and state persistence. Confirm
Learning at MM6 ID 30 with each proficiency, and no Grandmaster teacher or
bonus. Preserve MM7 behavior in its isolated runtime.

## Exact unresolved inputs for the consolidated F3 preflight

1. Obtain the six `StartingStats[class][stat]` vectors and
   `StartingSkills[class][skill]` codes from original MM6 or a trusted local
   reference. Cross-check distribution shape and every class-change reset.
2. Capture Quick Start's four identities, class/face/voice/stat/skill state,
   initial inventory, spell flags, gold/food, time, map position and quest
   flags. Distinguish it from the editable Create Party defaults.
3. Capture starting equipment and spellbook/known-spell behavior for each
   selectable base class/skill combination. Resolve every literal collision
   from the ignored probe against original behavior.
4. Obtain exact HP/SP bases and per-level/promotion formulas, mastery caps,
   teacher requirements and promotion quest rewards, including retroactive
   effects. Verify serialization IDs before freezing an MM6 save format.
5. Inventory MM6 portrait/voice/sex and class icon mappings and render the
   complete creation screen. Check graphical selection, text, item visuals,
   error state and save/reload before declaring the route playable.

None of these F3 inputs is closed by a documentation-only shape check or by a
matching enum label. Keep this lane alongside MM7/MM8 tables and UI rather
than substituting values in their shared runtime.
