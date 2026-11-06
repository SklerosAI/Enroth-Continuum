# MM6 text table consumer audit (F2)

Status: **literal-loader inventory, 2026-11-05**. This source-level check
compares the 30 retained MM6 text-table names with direct C++
`eventsData("name")` calls. It does not establish correct parsing, full row
coverage, title-specific semantics, or whether a table without a literal call
is unused. The reproducible `scripts/mm6_probe_table_consumers.py` reads only
the ignored retained catalog and tracked C++ source, prints aggregate counts,
and writes the detailed source-path/line map to ignored
`local-game-data/mm6/research-2026-11-05/table-consumer-probe.json` with input
and source fingerprints.

## Direct-loader coverage

Twenty-four of the 30 retained text-table names appear in a literal
`eventsData` call in the current source. Their consumers fall into these
groups:

| Group | Direct loaders | Required semantic check |
| --- | --- | --- |
| Character and UI text | `class.txt`, `skilldes.txt`, `stats.txt`, `global.txt` in `Localization.cpp` and other startup paths | MM6 row counts, field widths, class/skill ID meaning, text indices and visible UI. `class.txt` and `stats.txt` are prose, not numeric starting matrices. |
| Items and spells | `items.txt`, `rnditems.txt`, `stditems.txt`, `spcitems.txt`, `scroll.txt`, `spells.txt` in `MM6ItemTable.cpp`, `Engine.cpp` and table helpers | Every item/spell ID, effect, kind, price, treasure weight, scroll/book mapping, proficiency and cross-table reference. An MM6 loader can still feed an MM7 rule consumer. |
| World and services | `mapstats.txt`, `monsters.txt`, `2devents.txt`, `trans.txt`, `merchant.txt` in `Engine.cpp` and table helpers | Map/monster/house/transport linkage, schedules, prices, resets, encounters and title-specific gameplay consumers. |
| Quests, notes and NPCs | `quests.txt`, `awards.txt`, `autonote.txt`, `npcdata.txt`, `npcnames.txt`, `npcnews.txt`, `npcprof.txt`, `npctext.txt`, `npctopic.txt` in `Engine.cpp` and `NPCTable.cpp` | Every quest bit, award, autonote, NPC topic/profession/news and script use, including IDs absent from visible note text. |

These are static call-site groups, not a claim that every listed row is
consumed correctly. The MM6 branch still calls shared MM7 initializers and
has known numeric-ID collisions. For each table, F2 must identify its parser,
accepted schema, every runtime consumer, valid/reserved rows, and at least
one acceptance scenario per behavior family. The private catalog holds the
per-row evidence.

## Six tables with no literal loader found

The source scan found no literal `eventsData` load for `autonotes.txt`,
`npcbtb.txt`, `passwords.txt`, `proftext.txt`, `useitems.txt`, or
`errorlog.txt`. This is a **question**, not proof they are shipped gameplay
requirements or unused files. Search indirect resource paths and original
game behavior before assigning a purpose. In particular, compare the first
five with autonote display, NPC name/dialogue, Question/password events,
profession descriptions and item-use flows. Classify `errorlog.txt` as data
or incidental only after checking its provenance and consumers. Do not simply
wire every filename into startup.

The probe also compares the two retained autonote tables without publishing
their text. All 116 numeric IDs in `autonotes.txt` also occur in
`autonote.txt`; 114 shared rows have identical remaining cells, two differ,
and the singular table has 12 additional IDs. This suggests a close relationship,
but not which table the original game uses or whether the differing rows have
runtime significance. Resolve the provenance and observed behavior before
calling the plural table redundant.

### Focused structure and consumer findings

| Retained table | Source/data finding | F2/F3 question and later acceptance |
| --- | --- | --- |
| `useitems.txt` | Twenty-nine consecutive numeric item rows each have 35 fields. A header enumerates the same 29 item IDs across columns 6–34, forming a complete 29 × 29 symmetric matrix; all 841 mirrored pairs match and the diagonal is uniform. The IDs are in the MM6 herb/bottle item band. `MM6ItemTable.cpp` has no loader, while `UIPopup.cpp` uses an MM7 potion matrix and Alchemy gate. | Decode the matrix's 30 distinct result tokens, item/result IDs, order, failures and side effects. The structure strongly suggests MM6 item-combination rules, but token meanings and original behavior remain unverified. F4 must exercise all combinations offline and representative safe/failure/black-potion cases live. |
| `npcbtb.txt` | Twenty-four numeric rows include `%13` templates. `NPCTable.h` identifies `%13` as MM6-only and `BuildDialogueString` supports it, but no table loader was found. | Find its original trigger and text routing, then render a representative template with stable name substitution and save/reload where state matters. |
| `proftext.txt` | Its 77 numeric profession IDs all occur in loaded `npcprof.txt`, but the text columns differ. `NPCStats::InitializeMm6` fills hire/action/benefit/join fields and never sets `pDismissText`; inherited `UIDialogue.cpp` displays that field for a hired NPC, so that MM6 path can show blank dismissal dialogue. [MMExtension](https://grayface.github.io/mm/ext/ref/) documents `DismissText` as MM7-only, suggesting the intended MM6 dismissal flow differs. | Identify the MM6 dismissal flow and any role for `proftext.txt`. Test hire, service and dismissal for representative professions; do not assume the table supplies `pDismissText` until verified. |
| `passwords.txt` | Forty numeric rows and three columns, no loader. | Reconcile its relationship, if any, to MM6 Question/password events; test correct, incorrect and cancel branches without assuming this is their answer table. |
| `errorlog.txt` | One retained line and no loader. | Establish provenance before classifying it as incidental; do not add a game loader based on its name. |

The table matrix, overlap and column facts are from retained ignored content;
they do not copy game text or result tokens into Git. The private report should
hold the per-row comparison, source hashes and any future semantic mapping.
The owner's visually checked Prima guide chart matches the matrix's 25
unordered numeric-result input pairs, but two result rows disagree. The
tracked `scripts/mm6_probe_potion_guide.py` reproduces this comparison using
a private guide transcription and the private matrix report; it writes exact
recipes and differences only to ignored `potion-guide-reconciliation.json`.
The guide cannot settle the five marker meanings or original failure effects.
See [MM6 rules F2 closure](MM6_RULES_F2_CLOSURE.md).
`scripts/mm6_probe_unloaded_tables.py` reproduces these structural checks from
the retained references and writes detailed cells only to ignored
`local-game-data/mm6/research-2026-11-05/unloaded-table-probe.json`. Its
aggregate output confirms all 841 matrix cells are populated and symmetric,
with 25 distinct numeric candidate item IDs (all present in `items.txt` and
the matrix axis), four letter-digit markers, and one two-letter marker. The
diagonal uses the latter marker. These are lexical classes, not decoded
effects, recipes, or evidence that the current engine consumes the table.

## Closure checks

1. For each of the 24 directly loaded tables, compare its actual retained
   schema against the parser and every downstream title-specific use. A parse
   or nonzero row count is inventory evidence only.
2. For each of the six without a literal load, find an indirect consumer or
   evidence that the shipped file is unused/reserved/incidental. Keep that
   classification and its source in the private per-file manifest; summarize
   only the conclusion here.
3. Join table IDs with map, event, NPC, item, spell, media and save references.
   Resolve missing, duplicate and out-of-range links and model critical
   campaign branches before F3.
4. At F3, verify the frozen F2 table schema and link claims in one read-only
   original-file preflight. At F4, cover each active row family with an
   offline check or explicit reasoned review, then prove representative
   behavior in the graphical F5 game.

See also: [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md),
[class and skill specification](MM6_CLASS_SKILL_SPEC.md),
[event and campaign specification](MM6_EVENT_CAMPAIGN_SPEC.md), and
[local status](LOCAL_STATUS.md).
