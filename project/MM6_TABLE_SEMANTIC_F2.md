# MM6 text-table parser and consumer matrix (F2)

Status: **source and retained-copy audit, not semantic closure or runtime validation** (2026-11-05). This packet covers all 30 retained text-table families. It identifies the immediate parser and at least one downstream consumer or an explicit missing path. The F3 original-file preflight has not run. Exact rows, IDs, hashes, and exceptions stay in ignored `local-game-data/mm6/research-2026-11-05/table-semantic-probe.json`.

## Reproducible shape and reference check

`scripts/mm6_probe_table_semantics.py` reads the retained 30 files and current source. It records physical/numeric row shapes and current literal loader sites, then checks ten selected cross-table numeric edges. The aggregate has 24 files with a literal load and six without. All 367 positive NPC house references point to a retained house row, all 332 positive NPC profession references to a retained profession row, and all 298 first-three NPC topic references to a retained topic row. All 41 positive house exit-map references resolve to a listed map. Every one of the 50 scroll rows in the MM6 message-scroll loader range has an item template. One of the 82 total numeric `scroll.txt` rows lies outside `items.txt`, but is **outside** the loader's message-scroll range; this is not evidence of a reachable missing item.

These are reference-presence checks. They do not establish that the parser interprets every column, that a row is reachable, or that a referenced label has an executable MM6 action. In particular, the campaign audit found 74 NPC references with global functions intercepted by an inherited teacher range despite all 298 topic IDs being present as table rows.

## Thirty-family parser and consumer register

The `classification` column describes the **current fork's data path**, not a claim that the original game used or ignored every row. `Loaded` means a parser assigns some fields; `partial` means a source-visible row/range restriction or known semantic gap; `unloaded` means no direct literal loader and no indirect path confirmed. All rows still require MM6-specific semantic and F5 graphical checks where applicable.

| Retained table | Current parser and classification | Downstream consumer and acceptance obligation |
| --- | --- | --- |
| `2devents.txt` | `HouseTable.cpp::initializeHouses`, partial 24-column view; unknown type labels fall to one enum | `UIHouses`, `UITransition`, travel and event entry. Classify every active house/service, price, schedule, exit and video; inherited numeric house ranges cannot define MM6 services. |
| `autonote.txt` | `AutonoteTable.cpp::initializeAutonotes`, loaded text/type with default `misc` | `AutonotesBook`, character/event note writes. Prove all active note types, script IDs, visibility and persistence. |
| `autonotes.txt` | Unloaded plural variant; 116 IDs overlap singular, with two differing payloads | Resolve original provenance and whether plural text ever appears; do not discard on name alone. |
| `awards.txt` | `AwardTable.cpp::initializeAwards`, partial: rows without priority are skipped | `UICharacter` award pane and event/character award writes. Reconcile skipped rows, priority order, dynamic formatting and MM6 award IDs. |
| `class.txt` | `Localization.cpp::initializeClassNames`, positional descriptions; no numeric class matrix | Character creation and sheet still use inherited class-name/order logic. Verify six MM6 families, tier descriptions and all class selection/promotions. |
| `errorlog.txt` | Unloaded | Establish whether the shipped line is incidental or an original consumer before assigning any game role. |
| `global.txt` | `Localization.cpp::initialize`, numeric localization strings | All UI/status/dialogue text by `LstrId`. Check MM6 ID-to-meaning mapping, format placeholders, fallback overrides and visible screens. |
| `items.txt` | `MM6ItemTable.cpp::InitializeMm6`, partial: skips missing icon/invalid ID and maps kind/skill labels into shared enums | Inventory, equipment, treasure, shops, scrolls and potions. Compare every active item field, inherited category ranges, restrictions, render sprite and save/reload. |
| `mapstats.txt` | `MapTable.cpp::Initialize` has an MM6 branch for columns through music ID | Map lookup, encounter/reset/trap parameters and `AudioPlayer` music. Check all 67 map IDs, encounter names, resets and runtime transitions. |
| `merchant.txt` | `MerchantTable.cpp::initializeMerchants`, positional phrase zip | Shop and magic-guild dialogue. Confirm all phrase slots, item/service context substitutions and visible buy/sell/repair/identify text. |
| `monsters.txt` | `Objects/Monsters.cpp::MonsterStats::Initialize`, MM6 columns adapted to shared parser; known silent label fallbacks | `Actor` combat/spawn/loot. Resolve all active attack, spell, missile, resistance, family and hostility values before fight scenarios. |
| `npcbtb.txt` | Unloaded; retained `%13` templates have substitution support in `BuildDialogueString` | Locate MM6 beg/threat/bribe or other NPC text source and display a representative template with correct substitution. |
| `npcdata.txt` | `NPCTable.cpp::InitializeMm6`, positional 14-column row mapping with three event topics | House/street NPC selection, hire and dialogue. Prove house/profession/topic roles, dynamic placement, joins and saved NPC state. Presence of a referenced row alone is insufficient. |
| `npcnames.txt` | `NPCTable.cpp::InitializeNPCNames`, positional male/female name lists | Randomized NPC names. Check MM6 category/sex distribution, stable naming and save/reload. |
| `npcnews.txt` | `NPCTable.cpp::InitializeMm6` reads one text column into `pCatchPhrases` | NPC news dialogue. Determine whether `npcdata`'s mostly flag-like `newsTopic` is an ID or condition; verify regional news and repetition. |
| `npcprof.txt` | `NPCTable.cpp::InitializeMm6`, partial 8-column hire/action/benefit/join fields | Hired NPC cost, service effects, dialogue and dismissal. Verify all 77 profession rows and MM6-specific hired-NPC flow. |
| `npctext.txt` | `NPCTable.cpp::InitializeMm6`, text indexed as `id - 1` | NPC/global dialogue body. Reconcile ID offset with topic/event lookups, substitution and all campaign-critical branches. |
| `npctopic.txt` | `NPCTable.cpp::InitializeMm6`, topic label indexed as `id` | `NPCTopics` visibility/selection and `global.evt`. Classify global, MM6 special and text-only choices; fix inherited teacher precedence. |
| `passwords.txt` | Unloaded 40-row numeric table | Determine relation, if any, to MM6 Question/password events and test correct, wrong and cancel branches. |
| `proftext.txt` | Unloaded 77-ID profession variant | Resolve role versus `npcprof.txt`; MMExtension marks `DismissText` MM7-only, so do not force MM6 text into that field without evidence. |
| `quests.txt` | `QuestTable.cpp::initializeQuests`, text `0` becomes empty | `QuestBook` and event bit changes. Only a subset has visible note text; map all 208 script-used bits to producer, consumer and save state. |
| `rnditems.txt` | `MM6ItemTable.cpp::InitializeMm6`, partial: loops IDs 1–399 and positional footer chance rows | Treasure generation. Verify the additional numeric row's role, weight totals, level bands and generated item eligibility; do not infer effects from positive weights alone. |
| `scroll.txt` | `MessageScrollTable.cpp::initializeMm6MessageScrolls`, partial: only item IDs 500–549 are stored | `UIMessageScroll`. Reconcile 32 numeric rows outside loader range, multiline/quoted text and message-item use; the one row beyond `items.txt` is outside the loaded range. |
| `skilldes.txt` | `Localization.cpp::initializeSkillNames`, positional zip over inherited visible skills | Skill screen/tooltips. Verify MM6's 31 skill IDs, mastery descriptions and no Grandmaster claims or shifted names. |
| `spcitems.txt` | `MM6ItemTable.cpp::InitializeMm6`, fixed-position special enchantment rows | Item generation, value and bonuses. Check every active modifier, item-type chance and effect; shared bonus maps may encode MM7 semantics. |
| `spells.txt` | `Spells.cpp::SpellStats::Initialize` has an MM6 text branch but shared spell data/effects | All nine spell schools, cost, target, damage and effects. Check every ID and three mastery levels; no inferred effect from matching text. |
| `stats.txt` | `Localization.cpp::initializeAttributeNames`, 26 positional description slots | Character sheet. Verify MM6 stat/condition/resistance order, descriptions and format; not a numeric starting-stat source. |
| `stditems.txt` | `MM6ItemTable.cpp::InitializeMm6`, fixed-position standard enchantment rows and level footer | Item generation/bonus and price. Check all 14 attribute distributions and treasure-level ranges against item type and live loot. |
| `trans.txt` | `TransitionTable.cpp::initializeTransitions`, index to display description | `UITransition` renders text. This is **not** a stable/boat schedule table; find the MM6 route source and test actual travel separately. |
| `useitems.txt` | Unloaded; retained 29 by 29 item-combination matrix | `UIPopup` still uses MM7 potion rules. Decode five marker meanings and 25 numeric outcomes, settle two guide/result disagreements, cover all cells offline and representative mixing live. |

## Cross-table and consumer decisions still needed

1. For each positional parser (`class`, `skilldes`, `stats`, `merchant`, `npcnames`, enchantments), prove exact MM6 row order and width against the assigned destination enum or array. A row-count match alone cannot establish correct meaning.
2. For each keyed parser, validate IDs and duplicate/blank policy **before** writing to `IndexedArray` or a fixed array. The retained copy has mixed numeric, empty and header rows; one known source path silently chooses a fallback for unknown house labels.
3. Trace second-order references: map encounter monster names, all house types and room descriptors, item sprites/spell scrolls/books/wands, NPC news and profession effects, quest/award/note variables, and text formatting. The ten verified numeric joins above are only a small, chosen subset.
4. Resolve the six unloaded files by proving an indirect original/current consumer or an evidenced unused/reserved classification. The item matrix is a required MM6 implementation lead; the other five still need role/provenance checks. Do not add loaders simply because a file exists.
5. In F4, compare every active retained row against a title-specific parser and consumer, with strict malformed/unknown-token handling and scenario tests. In F5, inspect the resulting screens, services, combat, travel, quests, item use and save/reload. No current table parse alone can close a game feature.

See also: [MM6 table-consumer spec](MM6_TABLE_CONSUMER_SPEC.md), [MM6 rules closure](MM6_RULES_F2_CLOSURE.md), [MM6 campaign closure](MM6_CAMPAIGN_F2_CLOSURE.md), and [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md).
