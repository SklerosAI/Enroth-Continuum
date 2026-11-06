# MM6 original-guide F2 review ledger

Status: **chapter-level coverage and selected claim reconciliation; F2 semantic
review remains open** (2026-11-05). This ledger supplements the [guide index](MM6_GUIDE_RECONCILIATION.md).
It uses the owner's retained manual, reference card and Prima scan, the
retained table/event reports, and current C++ consumers. It does not reopen
the original game archives, verify every guide sentence or launch MM6.

## Method and source limits

`scripts/mm6_probe_guide_coverage.py` verifies the retained PDF identities,
page counts and extractable text per page, then writes per-page fingerprints
to ignored `local-game-data/mm6/research-2026-11-05/guide-coverage-probe.json`.
The run indexed all **61 manual, 5 card and 412 Prima PDF pages**. The
extractor marked **5, 0 and 35**, respectively, as pages with fewer than 100
text characters. Those are inspection flags, not missing-page counts. The
manual's PDF p. 57 is an illustrated New Sorpigal map; Prima PDF pp. 281 and
345 are illustrated essential-quest and map-appendix dividers. These three
pages were checked as rendered scans. Prima's printed page is approximately
PDF page minus eight after its front matter, confirmed at the chapter starts;
the ledger uses PDF pages for unambiguous locators.

The PDF text layer has broken words, especially in Prima. It can confirm a
chapter's location, but a spell value, recipe, map label or quest condition
needs visual scan and retained-data comparison. The private
`guide-f2-review-private.md` records detailed locators, selected observations
and disputes without adding guide prose or game rows to Git. Chapter-level
indexing is **not** a complete semantic or page-by-page review of 412 pages.

**Status vocabulary:** `confirmed` means a documented claim or source/consumer
collision has supporting retained evidence; it never means the MM6 game works.
`uncertain` means the guide/data meaning or live effect is unresolved.
`unsupported` means the current engine has no demonstrated MM6 path or has a
known incompatible inherited path. A row may contain all three.

## Coverage and acceptance ledger

| Guide family and PDF locator | Reconciliation status and current consumer | Specific F3/F4/F5 acceptance |
| --- | --- | --- |
| Story and setting: Prima pp. 11–22 | **Confirmed:** this is historical and campaign context, not a numeric rules table. **Uncertain:** guide narrative has not been joined event-by-event to story flags, movies and ending. See [campaign closure](MM6_CAMPAIGN_F2_CLOSURE.md) and [media closure](MM6_MEDIA_F2_CLOSURE.md). | At F3 classify original story media and event triggers; at F4/F5 verify opening, major story transitions and MM6 ending against shipped scripts and a persistent save. |
| Party creation: manual pp. 23–26; card p. 3; Prima pp. 23–34 | **Confirmed:** six base classes, four characters, shared 50-point pool, two fixed plus two chosen skills, class-based stat bounds; current `Party.cpp`, `UIPartyCreation.cpp` and `Character.cpp` use MM7 class, stat, skill and starting-item assumptions. **Uncertain:** exact starting arrays and Quick Start state. See [creation F2](MM6_CREATION_F2.md). | F3 obtain exact six starting vectors, legal skills and equipment/Quick Start state. F4/F5 create each class at min/max bounds, change class after edits, reject invalid choices, compare both start routes and save/reload. |
| Enemy catalog: Prima pp. 35–96 | **Confirmed:** retained `monsters.txt` has active damage, missile, spell and special labels that inherited parsing silently changes or drops. **Uncertain:** complete guide-to-row match, AI, resistance and drop effects. See [monster/combat spec](MM6_MONSTER_COMBAT_SPEC.md). | F3 resolve all active labels against original data and guide examples; F4 cover every active taxonomy/row; F5 fight melee, ranged, caster, status and neutral cases, then revisit and reload for loot/death persistence. |
| Input and screens: manual pp. 17–21, 30–33; card pp. 2–5 | **Confirmed:** card gives screen/control targets; inherited portrait names are absent from the retained MM6 icon index and the current creation screen has MM7 choices. **Uncertain:** dynamic image families and control equivalence on the Mac. See [media closure](MM6_MEDIA_F2_CLOSURE.md). | F3 identify MM6 art, portraits and voice mappings; F5 inspect every creation choice, game/character/inventory/book/map/dialogue screen, text and mouse/keyboard actions at the installed window scale. |
| Progression and skills: manual pp. 26–29, 40–41; Prima pp. 241–248, 311–326 | **Confirmed:** retained MM6 class/skill tables define three proficiency ranks and six three-tier class families; inherited four-ID family arithmetic and Grandmaster path conflict. **Uncertain:** starting arrays, exact training requirements, reward/HP/SP formulas. See [rules closure](MM6_RULES_F2_CLOSURE.md). | F3 settle arrays, teachers and promotion effects; F4/F5 train all active skills through permitted ranks, promote each family twice, check retroactive health/mana and reload. |
| Combat, recovery and calendar: manual pp. 37–40; card pp. 4–5 | **Confirmed:** guide sections cover turn/real-time play, rest and status; current monster parser has active MM6 label fallbacks. **Uncertain:** timing, damage, condition and recovery constants, including original behavior versus guide advice. | F3 record exact active rules/conditions; F4 test attack resolution and time/recovery transitions; F5 fight in both modes, rest with food, advance calendar, recover from conditions and reload. |
| Nine spell schools: manual pp. 42–54; Prima pp. 97–156 | **Confirmed:** retained `spells.txt` has 99 spell IDs and current spell dispatch is inherited; same-ID labels can conceal different effects and item-target routing. **Uncertain:** every spell's target, cost, mastery effects, duration, resistance, media and executable constants. See [spell effect F2](MM6_SPELL_EFFECT_F2.md). | F3 reconcile each original spell/scroll/book/monster/event reference with the guide and shipped data; F4 test all 99 at Normal/Expert/Master plus failure cases; F5 visually sample each school and verify save/reload. |
| Equipment, enchantments and alchemy: manual pp. 27–29, 33–36; Prima pp. 157–240 | **Confirmed:** retained item tables contain 581 IDs; `useitems.txt` is a 29×29 matrix with 25 safe numeric pairs. Visual chart review found two result disagreements with the retained table; current `UIPopup.cpp` uses MM7 potion/Alchemy rules. **Uncertain:** marker effects, disputed outputs and every equip/treasure effect. See [rules closure](MM6_RULES_F2_CLOSURE.md), [table semantics](MM6_TABLE_SEMANTIC_F2.md). | F3 check disputed pairs and marker semantics against originals; F4 cover item IDs/categories and all 841 combination cells; F5 equip, identify, repair, loot, mix success/failure and save/reload. |
| NPC followers: Prima pp. 249–254 | **Confirmed:** `npcdata.txt` references retained profession/topic/house rows, but a present topic is not an executable action; inherited teacher routing intercepts some MM6 global functions. **Uncertain:** hire effects, fees, dismissal and all dialogue branches. See [table semantics](MM6_TABLE_SEMANTIC_F2.md) and [campaign closure](MM6_CAMPAIGN_F2_CLOSURE.md). | F3 classify every active profession/topic and fee; F4 prove event dispatch versus special/text-only cases; F5 hire, dismiss, use profession effect and reload. |
| Shops, services and travel: manual pp. 34–37; Prima pp. 255–280 | **Confirmed:** `2devents.txt`, house/NPC tables and `trans.txt` have partial consumers; `trans.txt` is transition prose, not a complete stable/ship schedule. Inherited house type/video routing has MM6 mismatches. **Uncertain:** complete route timetable, pricing and service behavior. See [campaign closure](MM6_CAMPAIGN_F2_CLOSURE.md), [media closure](MM6_MEDIA_F2_CLOSURE.md). | F3 classify all house/service/transport records and schedules; F4 implement title-specific routes/prices; F5 enter representative shops, guilds, temples, training, taverns, banks, stables and ships, then reload location/time/gold. |
| Save and opening return: manual pp. 55–59 | **Confirmed:** the manual gives an opening-town walkthrough and its illustrated map; current `SaveLoad.cpp` writes MM7 snapshot types while `PathResolver.cpp` has an MM6 profile root. **Unsupported:** complete MM6 save encode/decode. See [world closure](MM6_WORLD_F2_CLOSURE.md). | F3 confirm MM6 delta and save records; F4 reject cross-title loads before mutation and preserve opaque fields; F5 save/quit/relaunch/load in opening town, after an indoor transition, quest reward and later checkpoint. |
| Essential campaign: Prima pp. 281–310 | **Confirmed:** retained quest/event reports expose candidate scripts and active branches for the council, Oracle, Varn, Control Center and Hive. **Uncertain:** quest-bit meaning and branch reachability; active MM6 opcodes/variables lack complete dispatch, and inherited Hive finale is MM7-specific. See [campaign closure](MM6_CAMPAIGN_F2_CLOSURE.md). | F3 resolve each guide milestone to exact original event/quest/map/house/item dependencies; F4 cover correct, wrong and cancel branches; F5 complete the six [guide milestones](MM6_GUIDE_RECONCILIATION.md) in game and reload between them. |
| Promotions and Seer: Prima pp. 311–326 | **Confirmed:** guide lists both promotion tiers for six classes; current character arithmetic and event handling are inherited. **Uncertain:** all reward and prerequisite joins. | F3 map every promotion/Seer guide entry to original script and class/award changes; F4/F5 complete each first and second promotion, rejection path and reward persistence. |
| Regional quests: Prima pp. 327–344 | **Confirmed:** retained events and quest rows exist, but keyword matching in the current graph is incomplete. **Uncertain:** every regional guide heading's script, NPC, item and map dependency. | F3 create a private one-to-one guide-heading/event dependency ledger; F4 cover all active branch families; F5 complete representative quests in every outdoor region and verify reward/reload. |
| Cartographic appendix: Prima pp. 345–412 | **Confirmed:** guide contains regional/dungeon maps; retained map index lists 15 outdoor and 52 indoor maps. **Uncertain:** every appendix label, entrance and in-map coordinate join; MM6 indoor loading and deltas are incomplete. See [world closure](MM6_WORLD_F2_CLOSURE.md). | F3 join every guide map to the 67 original map names and transitions; F4 validate geometry/delta links; F5 inspect entrances, indoor/outdoor collision, automap discovery and persistent chests/doors on representative maps. |

## Remaining gate

The chapter map is complete as a **locator index**. The full guide-to-engine
reconciliation is not: 99 spell effects, all item/monster rows, guide quest
headings, regional maps, service rules and OCR-weak pages still need direct
claim-level review with the scanned image and retained data. For every
disagreement, record page, source hash, table row or event offset, proposed
behavior and acceptance in the ignored private report. The [F3 preflight
matrix](MM6_F3_PREFLIGHT_MATRIX.md) then checks frozen F2 assumptions once
against the original installation; F3 has not run here.
