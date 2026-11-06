# MM6 guide-to-engine reconciliation

Status: **source coverage indexed; mechanic-by-mechanic reconciliation open**.
This file turns the owner's locally retained guides into testable MM6 engine
requirements. It is an index and gap register, not evidence that a mechanic
already works. The extracted game archives remain the authority for exact IDs,
payloads, and shipped content. Keep guide pages and game bytes in the ignored
local reference folder, never in this repository.

## Source locators

| Source | Locators used below | What it contributes |
| --- | --- | --- |
| [1998 New World Computing user manual](https://www.mocagh.org/nwc/mm6-manual.pdf) | Local 61-page PDF; **PDF page** numbers | Controls, party creation, rules, screens, spells, opening walkthrough, save/load |
| Owned five-page reference card | **PDF page** numbers | Game, character, inventory and creation screen diagrams; keyboard and mouse actions |
| [Prima official strategy guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) | Local 412-page PDF; **printed page** numbers from its contents (PDF page is about printed page + 8 after the front matter) | Full content categories, campaign path, promotions, regional quests and maps |
| [New World Computing limited edition guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf) | 11-page scan; **PDF page** numbers | Independent early-game and potion checks; this is a short supplement, not a full game catalog |

Page numbers were checked against the locally retained PDFs, including the
manual's page headings and Prima's contents and chapter starts. OCR has broken
words in places; a guide statement with an uncertain reading must be checked
against the page image and owned data before implementation.

The [F2 guide review ledger](MM6_GUIDE_F2_REVIEW.md) now maps 14 chapter
families to current consumers and acceptance checks. Its private page probe
indexed all 478 retained PDF pages, but that is a locator and text-extraction
check. It does not establish claim-by-claim coverage of every spell, item,
monster, quest, service or map. Those direct comparisons remain open here.

## Rules and interface coverage

| Feature family | Guide evidence | MM6 implementation and verification obligation |
| --- | --- | --- |
| New game and party | Manual PDF pp. 23–26; reference card PDF p. 3; Prima printed pp. 15–26 | Custom party and Quick Start; four characters; six MM6 classes and both promotions per class; names, portraits, stats, bonus points, starting skills/equipment, class restrictions and valid starting state. Verify the rendered UI and save/reload. |
| Input and screens | Manual PDF pp. 17–21, 30–33; reference card PDF pp. 2–5 | Keyboard and mouse movement, targeting, turn mode, ready spell, NPC slots, game/character/inventory screens, books, automap, right-click inspection and readable text. Exercise with keyboard and trackpad on this Mac. |
| Character progression | Manual PDF pp. 26–29, 40–41; Prima printed pp. 233–240, 303–318 | Class skill ceilings, experience, level training, skill-point costs, Expert/Master teachers and promotion rewards. Cross-check every class and skill against shipped `class.txt`, `skilldes.txt`, `stats.txt` and promotion events. |
| Combat, recovery and time | Manual PDF pp. 37–40; reference card PDF pp. 4–5 | Real-time and turn-based attacks, action recovery, quick spells, hit/miss/damage, rest and food, exhaustion, time/calendar, conditions and death/recovery. Check progression through a combat encounter and after save/reload. |
| Spells | Manual PDF pp. 42–54; Prima printed pp. 89–148 | All 99 spells across nine schools; MM6 ID/name, school, resistance, mana cost and effect at Normal/Expert/Master; target/range, duration, damage, recovery, restrictions and audiovisual feedback. Compare `spells.txt`, `scroll.txt`, monster spells and spellbook/item mappings with the engine's ID dispatch. |
| Items and alchemy | Manual PDF pp. 27–29, 33–36; Prima printed pp. 149–232; limited edition guide PDF pp. 3–4 | All equipment, consumables, scrolls, spellbooks, artifacts, relics and quest items; identify/repair, equipment limits, treasure generation, potion combination and failure cases. Shipped item tables decide exact IDs and values. |
| Economy, NPCs and travel | Manual PDF pp. 34–37; Prima printed pp. 241–272 | Shops and repair, guilds, temples, training, banks, taverns, councils, hired followers, stables and ships. Check schedule, price/reputation/skill effects, map transitions and persistence. |
| Save and return | Manual PDF pp. 55–59, especially pp. 56 and 58 | Save in the opening town, quit, relaunch and load; repeat after an indoor map transition, a quest reward, a promotion, a spell/item change and a later campaign checkpoint. Keep the MM6 profile separate from MM7 and MM8. |

The reference card diagrams are useful as visual targets, but their existence
does not prove the present UI has the right layout. The manual's spell prose
and the guide's numeric claims need comparison with shipped data and actual
effects; cataloged spell names alone do not satisfy this table.

## Campaign coverage and acceptance path

The Prima contents separate essential quests (printed pp. 273–302), class
promotion and Seer quests (pp. 303–318), regional quests (pp. 319–336), and
map appendix (p. 337 onward). Every listed quest must be linked to a shipped
quest/event ID and its map, item, NPC, house or transition dependency. In
particular, these are milestone scenarios for a complete campaign:

1. **Opening, manual PDF pp. 55–59.** Create or Quick Start a party in New
   Sorpigal, interact with the town and council, enter Goblinwatch, obtain and
   turn in its objective, train, save, quit and reload. This checks basic UI,
   outdoor/indoor maps, combat, doors, chests, NPC dialogue, quest flags and
   transition persistence.
2. **High Council and traitor, Prima printed pp. 273–286.** Resolve all six
   council subquests and the admission gate in Free Haven; verify Nicolai and
   traitor branches. Check each vote/event flag and the ability to progress
   without a prescribed order where the shipped scripts allow it.
3. **Oracle, Prima printed pp. 287–292.** Retrieve and place the four memory
   crystals, then verify the Oracle unlocks the next route. Confirm item and
   flag persistence through save/reload.
4. **Tomb of Varn and Control Center, Prima printed pp. 293–298.** Verify the
   Varn access puzzle, required information/items, Control Center transition,
   enemies and equipment progression. Exercise both maps and their deltas.
5. **Archibald and Hive ending, Prima printed pp. 298–302.** Verify the
   release chain, the Ritual of the Void dependency, Hive access, reactor and
   Queen encounter, ending movie/state, and a loadable save before the finale.
6. **Promotions and regional play, Prima printed pp. 303–336.** Cover both
   promotions for each of the six starting classes, the Seer quest, and each
   regional quest family across the guide's outdoor grid. Verify the
   corresponding rewards and trainer/house availability.

The guide organizes a route; the shipped EVT scripts decide actual branching,
flags and destinations. Do not hardcode a walkthrough order or infer that a
quest is implemented merely because its text, map or event record can parse.

## Reconciliation questions before the single original-data preflight

- **Spell costs and effects.** The inherited engine's `SpellId` and spell-data
  tables are MM7-oriented. The MM6 `spells.txt` describes its own ID/name and
  three-rank costs; cross-check the full 99-row table, cast switch, scroll and
  monster pathways. A displayed MM6 spell name can conceal an MM7 effect.
- **Class and skill rules.** The manual's six starting classes and
  Normal/Expert/Master progression must be reconciled with `class.txt`,
  `skilldes.txt`, trainer houses and promotion events. MM7-only classes or
  Grandmaster rules cannot be assumed for MM6.
- **Item and potion claims.** Use `items.txt`, `rnditems.txt`, `stditems.txt`,
  `spcitems.txt`, `useitems.txt` and actual recipe behavior to check guide
  descriptions. The limited edition guide's short potion section is a useful
  cross-check, not an authority for all recipes or modern GOG data.
- **Quest and map names.** Prima's printed map grid and quest headings must
  match `mapstats.txt`, `quests.txt`, `global.evt` and each listed-map EVT/STR.
  OCR names and guide map labels are not binary IDs.
- **Media and finale.** Check guide references to story events against the
  archive video list and actual triggers. The inherited MM7 house animation
  table does not resolve MM6 house videos; no movie event should be invented
  from a bitmap command.

For every disputed claim, record the guide page, shipped row or event offset,
chosen behavior and reason in the private reconciliation report. The F2 gate
is satisfied only when every guide category above has a linked data/engine
consumer and an acceptance check or an explicit, evidenced exception. The F3
preflight then reopens the originals in one deliberate read-only pass to
validate that frozen specification before the full MM6 implementation pass.
