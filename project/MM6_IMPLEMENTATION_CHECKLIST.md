# MM6 implementation checklist

Status: **inventory in progress, 2026-11-05**. This is the public, asset-free
index. The exhaustive per-file and per-row catalog is private at
`local-game-data/mm6/research-2026-11-05/catalog.json` and is ignored by Git.
An inventoried row is not an implemented game rule. Check an item only when a
title-specific implementation and an appropriate offline or live check exist.

Focused F2 specifications: [guide and campaign evidence](MM6_GUIDE_RECONCILIATION.md),
[events, dialogue and campaign](MM6_EVENT_CAMPAIGN_SPEC.md),
[class, skill and party creation IDs](MM6_CLASS_SKILL_SPEC.md),
[save and map-delta layout](MM6_SAVE_SPEC.md),
[presentation and media routing](MM6_MEDIA_UI_SPEC.md),
[text-table consumers](MM6_TABLE_CONSUMER_SPEC.md), and
[monster/combat rules](MM6_MONSTER_COMBAT_SPEC.md). These are audit inputs, not
claims that their runtime behavior is implemented.

## Evidence and boundaries

- [x] Preserve the MM7 source baseline on `mm7-isolated-runtime`; independent
  Atlas build launches its own `MM7.app` and loads the existing MM7 save.
- [x] Copy and hash the local MM6 LOD archives into an ignored reference set.
  Current archive lists contain 2,703 icon, 134 map, 1,958 bitmap and 4,314
  sprite entries. Extraction produced 9,128 files including manifests; image
  conversion and generated names mean extracted count is not an archive count.
- [x] Catalog every extracted file with size and SHA-256, plus every nonempty
  row of the 30 text tables, locally. This is raw coverage, not semantic review.
- [x] Generate ignored, reviewable TSV indexes for all item, spell, monster,
  map and quest-bit IDs. Numeric sequences are continuous: item 0–580, spell
  1–99, monster 1–173, map 1–67 and quest bit 1–512. Item templates 0, 299 and
  399 have blank names; only 52 quest-bit rows contain visible quest-note text.
  Blank rows still need use-site review before calling them unused.
- [x] Extract text locally from the owned 61-page manual and 5-page reference
  card. Both source PDFs and text are ignored; their sections still need full
  mechanic-by-mechanic review.
- [x] Inventory the installed video, sound, music and executable file families:
  `Anims1.vid` has 40 entries, `Anims2.vid` has 87, `Audio.snd` has 1,526,
  and `Sounds/2.mp3` through `16.mp3` provide 15 music tracks. Archive and
  executable hashes, and per-track music hashes, are in the ignored research
  folder. The original executable is PE32 x86; it is reference material only.
- [x] Match the `mapstats.txt` Redbook track column to the local MP3 filenames:
  all 67 maps select an installed track. Twelve distinct track IDs are used by
  maps; the remaining MP3s need event or menu mapping checks.
- [ ] Decode media entry formats and map all movie, effect and music IDs to
  game behavior; inspect native MM6 save format if a lawful local sample exists.
- [ ] Read the owned original manual and reference card and annotate every
  relevant mechanic. Compare the [original manual](https://www.mocagh.org/nwc/mm6-manual.pdf),
  [New World Computing guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf),
  [Prima guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) where accessible,
  and [MMExtension reference](https://grayface.github.io/mm/ext/ref/).
- [x] Index the owned manual, reference card, Prima guide and short NWC guide
  by mechanic and campaign milestone in
  [MM6_GUIDE_RECONCILIATION.md](MM6_GUIDE_RECONCILIATION.md). Page locators
  and acceptance scenarios are recorded; detailed rule-by-rule comparison
  with shipped tables and actual effects remains open.
- [ ] Reconcile conflicting guide claims against the shipped GOG data. Keep
  guide citations and disputed claims here; never publish copied guide text.
- [ ] Run a single deliberate read-only original-archive preflight after this
  specification is complete. Record all counts, schema lengths, ID gaps,
  boundary samples and unresolved differences in the ignored local report.

## Shipped table coverage

Every source row is indexed in the private catalog. The numbers below are raw
nonempty rows; headers, separators and reserved records still require review.

- [x] Compare the 30 retained table names with literal C++ `eventsData` loads.
  `scripts/mm6_probe_table_consumers.py` records direct source-path/line hits
  and source fingerprints only in an ignored report. Twenty-four names have
  direct loads; six have none. [MM6_TABLE_CONSUMER_SPEC.md](MM6_TABLE_CONSUMER_SPEC.md)
  identifies the groups and open questions. This is not evidence that a
  loader understands every MM6 row or that an unreferenced table is unused.
- [ ] Give every table a verified parser/schema, title-specific consumer,
  active/reserved/unused classification, and acceptance scenario. Resolve
  indirect consumers or documented non-use for the six without literal loads
  before the consolidated F3 preflight.
- [x] Identify the retained `useitems.txt` two-axis structure: 29 item-ID
  rows and matching header IDs form a complete symmetric 29 × 29 matrix in
  columns 6–34. No MM6 loader consumes it, and its 30 distinct result tokens
  remain private and semantically unclassified.
  `scripts/mm6_probe_unloaded_tables.py` reproduces the matrix, table-overlap
  and shape checks with detailed evidence in an ignored report. Its lexical
  classes include 25 candidate item IDs and five marker codes, with effects
  still unknown. The
  [table-consumer specification](MM6_TABLE_CONSUMER_SPEC.md) records this as
  a strong item-combination lead, not a finished recipe implementation.
- [ ] Decode the `useitems.txt` token/result vocabulary, recipe and failure
  effects, and any non-potion item use; reconcile with the owned guides and
  original behavior in F3, then cover all matrix cells and representative
  live mixes in F4/F5. Also resolve the absent profession dismissal text and
  the `npcbtb.txt`, `passwords.txt` and autonote-variant roles.

| File | Raw rows | Required reconciliation |
| --- | ---: | --- |
| `items.txt` | 582 | IDs 0–580; equipment, consumables, quest items, artifacts, enchantment and sprite linkage |
| `rnditems.txt` | 411 | Random treasure levels, weights, special cases |
| `stditems.txt` | 26 | Standard modifiers and price/value rules |
| `spcitems.txt` | 67 | Special modifiers and effect rules |
| `useitems.txt` | 38 | Usable item actions and restrictions |
| `spells.txt` | 108 | Spell IDs 1–99, nine schools, costs, recovery, duration, targeting and all three mastery levels |
| `scroll.txt` | 83 | Spell scroll mapping and casting behavior |
| `skilldes.txt` | 32 | Skills and mastery descriptions |
| `class.txt` | 19 | Class descriptions and promotion order; starting-skill/equipment matrices are not supplied by this prose table |
| `stats.txt` | 26 | Stat descriptions; numeric starting limits and derived formulas need other evidence |
| `monsters.txt` | 176 | Monster IDs 1–173, AI, attacks, spells, drops and resistances |
| `mapstats.txt` | 70 | Map IDs 1–67, reset, encounter, travel and map file linkage |
| `2devents.txt` | 559 | House, shop, service and other 2D event IDs |
| `global.txt` | 597 | Global event text and state |
| `quests.txt` | 513 | Quest text IDs 1–512 and progression flags |
| `awards.txt` | 101 | Awards and promotion state |
| `autonote.txt` | 129 | Auto note linkage |
| `autonotes.txt` | 117 | Auto note text |
| `merchant.txt` | 7 | Merchant behavior and price adjustment |
| `npcdata.txt` | 400 | NPC identities, location, join/hire data |
| `npcnames.txt` | 541 | Name catalog and local assignment |
| `npcprof.txt` | 80 | NPC professions, services and effects |
| `proftext.txt` | 79 | Profession descriptions |
| `npcbtb.txt` | 28 | NPC base text blocks |
| `npcnews.txt` | 281 | News text and conditions |
| `npctext.txt` | 521 | Dialogue text and event linkage |
| `npctopic.txt` | 527 | Dialogue topic mapping |
| `passwords.txt` | 41 | Password and entry event mapping |
| `trans.txt` | 234 | Transport destinations, schedules, costs and flags |
| `errorlog.txt` | 1 | Diagnose whether shipped data or incidental log; no gameplay assumption |

For every numeric ID, record whether it is active, reserved, duplicated or
missing. Record every nonnumeric row by file and line. Each table needs a
parser, a consumer, and a coverage check; a row count alone does not satisfy
that requirement.

### Monster and combat reconciliation

- [x] Compare all 173 MM6 monster-table rows and the retained fixed-size
  description table with the current loader and shared combat parsers.
  [MM6_MONSTER_COMBAT_SPEC.md](MM6_MONSTER_COMBAT_SPEC.md) records source paths
  and F3 questions. `scripts/mm6_probe_monster_rules.py` reproduces aggregate
  lexical mismatches in an ignored report. It finds 49 nonzero attack-type
  cells falling to physical, 46 missile cells falling to `NONE`, 13 monster
  spell cells falling to `SPELL_NONE`, and 11 special-attack cells falling to
  `NONE`. This is parser inventory, not observed combat behavior.
- [ ] Give MM6 title-specific monster IDs/families, projectiles, damage and
  resistance channels, special attacks, spell casts, hostility/faction rules,
  treasure and fixed drops. Eliminate silent fallbacks for active labels and
  prove representative melee, ranged, caster, flying, neutral and loot cases
  in offline scenarios and live F5 play.

### Spell and item ID reconciliation

- [x] Compare all 99 shipped MM6 spell rows with the inherited `SpellId`
  labels and `pSpellDatas` mana costs. `scripts/mm6_probe_spells.py` records
  the full per-ID comparison in the ignored `spell-reconciliation.json`:
  50 names match after simple normalization, 21 appear at another inherited
  ID, and 28 are absent or differently named. These are lexical categories,
  not proof that matching names have matching effects. The first three mana
  costs differ for 27 IDs. The MM6 `Elec`, `Cold` and `Pois` damage tags on 11
  spells currently fall through `SpellStats::Initialize` to physical damage.
- [ ] Implement MM6-specific spell IDs, casting effects, three mastery levels,
  costs, recovery, damage/resistance types, target rules, buff duration,
  scroll/book/wand mappings, monster/event casts, visuals and descriptions.
  Verify every ID 1–99 against the owned table and guide, with representative
  live casts across all nine schools; do not route MM6 ID 2 (Flame Arrow)
  through the inherited ID 2 Fire Bolt effect.
- [x] Compare the retained MM6 `items.txt` categories with inherited MM7
  `ItemId` ranges. `scripts/mm6_probe_item_ranges.py` records all 581 rows in
  the ignored `item-range-probe.json`. MM6 has 99 spell scrolls at 200–298,
  99 spellbooks at 300–398, and 50 message scrolls at 500–549. Inherited
  `spellForScroll` indexes 300–398, `spellForSpellbook` indexes 400–498,
  and the MM7 artifact/special range begins at 500. The wand range 135–159
  aligns numerically, but each wand's spell still needs MM6-specific mapping.
- [ ] Replace MM7 item-kind range assumptions with MM6-specific IDs and
  checks. Make every MM6 spell scroll cast its intended spell, every book
  teach its intended spell, every message scroll display its text, and every
  artifact/relic apply its own effect. Verify treasure generation, shops,
  pickup, inventory, equip/use and save/reload for each category.
- [x] Trace inherited creation, class/skill, potion and combat consumers in
  [MM6_CLASS_SKILL_SPEC.md](MM6_CLASS_SKILL_SPEC.md). `Party::Reset` and
  `createDefaultParty` seed MM7 classes, faces, skills and opening state;
  inherited teacher/UI paths expose Grandmaster; potion mixing uses MM7
  potion IDs and an Alchemy skill absent from MM6's table. This is source
  evidence of mismatches, not MM6 rule implementation.
- [ ] Reconcile exact MM6 Create Party and Quick Start starting matrices,
  six-class attributes/HP/SP, promotion adjustments, 31 skill availability
  and Expert/Master teachers, Learning XP, potion recipes and effects, and
  MM6 damage/resistance formulas against owned documentation, shipped data
  and acceptance scenarios. Class and stats prose tables do not supply every
  numeric constant. Test the official 50-point pool and attribute bounds,
  class-change reset, separate Quick Start fixture, starter letter/equipment/
  spellbooks, MM6 herb/bottle classification, mixing failures and black-potion
  persistence. The current MM7 potion predicates exclude MM6's herb/bottle
  range and its Alchemy gate is inapplicable.

## Archive and map formats

- [ ] LOD directory, compressed payload and case-insensitive lookup semantics.
- [ ] Icon, bitmap and sprite formats, palettes, transparency, animation frames,
  equipment positions, character faces and title-specific UI resources.
- [x] Verify the 67 map filenames in `mapstats.txt` match the 67 base files in
  `Games.lod`, with every 15 outdoor and 52 indoor file having its delta pair.
  All 134 retained map entries have valid size headers and their raw DEFLATE
  streams produce exactly the declared sizes. Strict zlib validation fails on
  `cd3.blv` and `d08.blv`: their complete DEFLATE streams are followed by 2,500
  and 1,385 extra bytes respectively before the final four bytes. Determine
  why those bytes are present and define a bounded runtime decoding policy.
  The ignored `map-probe.json` records entry and decoded hashes.
- [x] Validate the section boundaries of all 52 retained `.blv` files and
  their 52 `.dlv` companions against MM6's distinct record sizes. Every
  decoded file parses exactly to its declared end; the ignored
  `indoor-probe.json` records section counts and hashes. MM6 deltas have an
  8-byte opening header, 875 visible-map bytes, 548-byte creatures, 100-byte
  items, 20 fixed 4,204-byte chests, and 200 fixed 80-byte door slots plus
  variable door data from the base map. The per-door variable-array counts
  exactly equal each base map's declared door-data size. Format cross-check:
  [MM8LevelEditor's MM6 indoor reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/IndoorDataMap.java)
  and [delta reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/DeltaIndoorDataMap.java).
- [x] Validate the section boundaries of all 15 retained `.odm` files and their
  15 `.ddm` companions. Every decoded file parses exactly to its end; the
  ignored `outdoor-probe.json` records section counts and hashes. The base
  maps contain 921 model headers and 37,187 model faces in aggregate, with
  zero BSP nodes declared by those models. MM6 outdoor deltas start with an
  8-byte header and two 968-byte visibility arrays, then use 548-byte
  creatures, 100-byte items, 20 fixed 4,204-byte chests, 200 map-bit bytes
  and a 56-byte tail. Format cross-check:
  [MM8LevelEditor's MM6 outdoor delta reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/outdoor/DeltaOutdoorDataMap.java).
- [ ] All 15 `.odm` outdoor maps and 15 `.ddm` delta companions: geometry,
  terrain, objects, actors, doors, chests, map events and persistence.
- [ ] All 52 `.blv` indoor maps and 52 `.dlv` delta companions: geometry,
  portals, rooms, lights, objects, actors, doors, chests and persistence.
- [x] Consolidate the observed MM6 delta layout, inherited MM7 save paths and
  isolated `.ec6` save contract in [MM6_SAVE_SPEC.md](MM6_SAVE_SPEC.md). All
  67 retained delta companions reach their exact decoded end. The 200
  persistent bytes and 56-byte tail still need field-level semantics and a
  lossless write/read check; native `.mm6` save compatibility is unproven.
- [x] Trace MM7-only reset, door/chest reconstruction, transition, load-menu
  and write paths in the [save specification](MM6_SAVE_SPEC.md). Current
  `SaveLoad.cpp` updates in-memory map deltas before a direct file write, and
  the MM6 path still uses MM7 snapshot and filename types. Plan all-map
  lossless re-encoding, transition/re-entry checks and atomic-write failure
  cases before F4; source inspection alone does not prove persistence.
- [x] Frame all 67 listed map EVT files and `global.evt`: 14,477 records, 43
  distinct opcodes, no framing errors. The other 15 EVT-named files are not
  listed maps; one (`d4bm.evt`) is not a framed script and must not be fed to
  the current event parser. The ignored `event-probe.json` retains every record
  and the payload-length set for each active opcode.
- [x] Check unlisted EVT files against every archived `.odm`/`.blv` basename
  and the runtime's event lookup. None has a matching base map; the current
  engine loads only `global.evt` and the active map's basename plus `.evt`.
  Keep these files in the private catalog, but do not feed them to the active
  parser unless a new path to them is found.
- [ ] Decode and implement every listed-map/global EVT operand and side effect.
  Specifically, active opcodes 20, 27 and 28 are absent from the current C++
  enum. The [MM6 level editor's event format source](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java)
  labels them item modification, random password and random answer but marks
  their argument meanings unverified. The owned scripts confirm respective
  payload lengths of 5, 2 and 1 bytes. Opcode 20 occurs seven times in
  `znwc.evt`; 27 and 28 occur once each in `d09.evt`. Determine field meanings
  and game behavior before implementing them.
- [x] Compare all 14,477 active EVT record frames against the current C++
  parser's MM6 minimum sizes, missing cases and known fixed-width consumers.
  The reproducible `scripts/mm6_probe_event_parser.py` records 318 certain
  parser failures in the ignored `event-parser-probe.json`: 272 opcode-19
  monster spawns are shorter than the inherited minimum; nine records use the
  absent opcodes 20, 27 or 28; six opcode-24 and 19 opcode-26 records would
  leave bytes unread; and 12 opcode-12 records would leave bytes under the
  inherited movie interpretation. This is a lower bound, not proof that the
  remaining 14,159 records have correct meanings or execution behavior.
  [MM8LevelEditor's MM6 event source](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java)
  supports the 15-byte monster-spawn shape (three one-byte fields plus three
  four-byte coordinates), the 13-byte monster-modification shape (three
  integers plus one byte), and a distinct input-dialog operand set. Verify
  their semantics against the owned scripts before implementation.
- [x] Reconcile MM6 Question and NPC-topic structure with current consumers
  in [MM6_EVENT_CAMPAIGN_SPEC.md](MM6_EVENT_CAMPAIGN_SPEC.md). All 19 active
  opcode-26 Question records have a 13-byte shape with three in-range local
  string references and an existing target step; the inherited parser reads
  one field and the interpreter asserts. A retained NPC comparison finds 35
  labeled topic references without a global event or current special handler.
  `scripts/mm6_probe_dialogue_events.py` reproduces the aggregate counts and
  writes the per-record evidence only to an ignored local report. These are
  structural findings; the intended answer and dialogue behavior still
  require semantic review.
- [ ] Classify every nonzero MM6 NPC topic as a global event, MM6-specific
  special action or evidenced text-only choice. Define Question answer/cancel
  branches, the missing opcode effects, and a private quest dependency graph
  through the ending. Quest-note text covers only a subset of the script-used
  quest bits, so do not infer campaign state from visible notes alone.
- [x] Build a private structural graph of active event functions, map moves,
  house exits, quest-bit operands and NPC topic changes. The reproducible
  `scripts/mm6_probe_quest_graph.py` records detailed links only in ignored
  `quest-graph-probe.json`. All 93 explicit script map targets and 41 house
  exit targets resolve to listed MM6 maps; four quest bits are read without a
  script write and 70 are written without a script read. These are structural
  leads, not gameplay defects or proof of ending reachability.
- [ ] Resolve the graph's guide milestone candidates against actual EVT
  conditions, item/NPC/house effects, alternate routes and persistent state;
  demonstrate a New Sorpigal-to-ending route and required promotions with
  scenario checks before F4 is considered complete.
- [x] Inventory the one-byte variable IDs in all active MM6 compare/add/
  subtract/set events. The reproducible `scripts/mm6_probe_event_variables.py`
  records 4,285 references to 127 distinct IDs in the ignored
  `event-variable-probe.json`, with event offsets, values, source hashes, and
  both MM6 and inherited enum labels. Of these references, 1,609 use IDs 46
  or above where MM6's variable vocabulary diverges from MM7's; 1,240 are
  MM6 map bytes 1–100, 222 are MM6 party-state/autonote variables, 54 are
  conditions, 40 are skills, and 53 are MM6's resistance taxonomy. Concrete
  collisions include MM6 ID 105 (map byte 1) versus inherited `VAR_Cursed`,
  and MM6 ID 205 (event autonote) versus inherited decoration variable 7.
  The [MM6 editor's target-ID list](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java)
  supplies the vocabulary; the owned scripts establish which IDs are active.
- [ ] Give MM6 its own variable-ID decoder, validation and execution semantics
  for compare/add/subtract/set, including 100 map bytes, condition and skill
  IDs, resistance mapping, party-state values and persistence. The current
  `PersistentVariables`/`PersistentVariables_MM7` layout has 75 map bytes and
  125 decoration bytes; do not reinterpret MM6 bytes through the MM7 enum or
  save them in the MM7 layout. Review all active variable IDs and values in
  the private catalog, then prove their effects in representative campaign
  events and MM6 save/reload checks.
- [x] Pair all 67 listed maps with their `.str` and `.evt` files and check
  established local-string operands against the NUL-delimited string count.
  Of 2,925 such references, 2,400 are in range and 518 out-of-range mouseover
  values belong to house events where the current engine uses the house name.
  Seven mouseover values have no such override and would display no hint under
  the current lookup. Opcode 5 occurs 56 times; the MM6 editor describes its
  one-byte operand as unknown, so it is inventoried separately rather than
  assumed to index local strings. The ignored `string-probe.json` keeps the
  exact map/event IDs and all nine unlisted `.str` names.
- [ ] Resolve the seven direct mouseover exceptions, opcode 5 semantics, and
  use of the nine unlisted `.str` files, including `intro`, `win` and `lose`.
- [x] Validate record boundaries of all 11 shipped `.bin` resources. Ten use
  a four-byte count followed by fixed-size records; `dsft.bin` has two counts,
  6,455 sprite frames and 1,656 extended-frame indices. The ignored
  `binary-table-probe.json` records counts, lengths and hashes. `dchest.bin`
  has eight 36-byte records; every MM6 row declares a 14×10 chest grid and
  image IDs 1–8. The current runtime does not load it.
- [x] Validate all 14 `.fnt` headers and glyph bounds against the runtime's
  two font atlas layouts. Thirteen use the MM7-style metrics layout; `calig.fnt`
  uses the older width-only layout. The ignored `font-probe.json` records each
  format. `calig.fnt` and `legal.fnt` have no direct `LoadFont` call; determine
  whether MM6 needs either in its UI.
- [ ] Map every binary-table record and font to title-specific behavior and
  verify the resources rendered by the MM6 interface.
- [x] Cross-check retained media archive entry names and soundtrack manifest
  against MM6's map music IDs and `dsounds.bin`: all 12 distinct map track IDs
  have corresponding numbered MP3 files; the sound table has 1,355 rows and
  seven names absent from the 1,526-entry retained `Audio.snd` index. Determine
  whether these are unused, aliases or errors before assigning fallbacks. The
  reproducible `scripts/mm6_probe_media_references.py` writes the ignored
  `media-reference-probe.json`, including input hashes and per-record evidence.
- [x] Trace the current video, house, sound, music and character-UI consumers
  against the retained MM6 name indexes. [MM6_MEDIA_UI_SPEC.md](MM6_MEDIA_UI_SPEC.md)
  records source paths and F3 questions. The retained video index has
  extensionless names, while `MPlayer::LoadMovie` appends `.bik` or `.smk`;
  whether the index preserved raw VID names requires F3 verification. Three
  sound IDs occur more than once in `dsounds.bin`, and the MM6 table loader
  currently keeps the last row for each. This is source-level evidence only;
  no MM6 media playback has been checked.
- [ ] Video and audio archive structure, house/movie routing, playback and
  fallbacks. The 116 distinct MM6 house animation IDs used in `2devents.txt`
  are within the inherited 196-entry MM7 `pAnimatedRooms` array, but **zero**
  of their MM7 video names occur in the retained MM6 `Anims1`/`Anims2` indexes.
  Build an MM6-specific room descriptor table rather than indexing the MM7 one.
- [x] Reconcile active EVT opcode 11/12 resource names with the retained MM6
  bitmap and video indexes. All 215 opcode-11 and 12 opcode-12 names resolve
  to `BITMAPS`, not to the video indexes. Opcode 11 has a four-byte field before
  its bitmap name; opcode 12 has two four-byte fields before its bitmap name.
  The current MM7 `EVENT_ShowMovie` interpretation of opcode 12 is therefore
  wrong for these shipped MM6 records. The
  [MM6 editor source](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java)
  also flags a possible opcode-11/12 texture/movie swap; the owned record
  shapes and bitmap-name matches are stronger evidence for the active scripts.
- [ ] Determine the exact two field meanings and side effects of MM6 opcode
  12, implement its title-specific parser/executor, and identify how MM6
  invokes movies outside these active map events.
- [ ] Compare one low, middle and high record of every ID family and every
  structure boundary with MM7. Record incompatible assumptions explicitly.
- [x] Record the retained MM6 class and skill IDs, source row widths, known
  inherited collisions and creation UI asset families in
  [MM6_CLASS_SKILL_SPEC.md](MM6_CLASS_SKILL_SPEC.md). MM6 has six three-tier
  class families (IDs 0–17) and 31 skills (IDs 0–30); inherited MM7 skill
  ID 30 is Dodge where MM6 has Learning. Affinity, HP/SP and portrait/voice
  semantics remain to be established and implemented.

## Engine and campaign behavior

The current MM6 branch is a format adapter, not a complete engine. Source
review points to these concrete inherited MM7 assumptions to remove or prove
correct before the F4 readiness gate:

| Runtime area | Current evidence | Required MM6 work |
| --- | --- | --- |
| Startup and resources | `Engine.cpp` still enters `MM6_Initialize`, `MM7_LoadLods` and `MM7_Initialize` | Audit each initializer and make title-specific asset, layout and rule paths explicit |
| Items | `MM6ItemTable.cpp` reads MM6 tables but still populates shared special/artifact bonus maps. MM6 scrolls, books and messages occupy the inherited MM7 scroll, book and artifact ID bands respectively. | Replace all title-specific item-kind, spell mapping and bonus assumptions; verify every MM6 item ID, special effect, potion recipe, enchantment and equipment rule. |
| Spells | MM6 spell names differ from inherited numeric labels at 49 of 99 IDs under a strict lexical check; 27 IDs have different mana triples, and 11 nonphysical MM6 damage tags currently fall back to physical. | Implement MM6's own 99 spell definitions and effects across every school and mastery, including the linked MM6 scroll/book/wand mappings. |
| Chests | MM6 `dchest.bin` declares 14×10 grids; `ChestTable.cpp` currently hardcodes eight 9×9 MM7 layouts. The 4,204-byte MM6 delta chest record accommodates 140 28-byte items and 140 short grid cells. | Load MM6 chest dimensions and images, reconstruct MM6 chest items and matrices, and verify placement, opening and save/reload without changing MM7 chests. |
| Outdoor maps | `MM6OutdoorSnapshot.cpp` maps MM6 data into MM7 structures. Offline probes confirm all 15 base/delta layouts; all 921 owned model headers declare zero BSP nodes. `OutdoorLocation::Load` still deserializes `OutdoorDelta_MM7`. | Implement MM6 delta decoding and reconstruction; prove geometry, collision, decoration, spawn, events and persistence across all 15 maps. |
| Indoor maps | `IndoorLocation::Load` decodes both `.blv` and `.dlv` as `IndoorLocation_MM7` / `IndoorDelta_MM7`; no MM6 indoor snapshot reader exists. Offline probes now confirm all 52 MM6 base and delta section layouts, including MM6's shorter header and record sizes. | Implement MM6 decoding, reconstruction, respawn and delta persistence for all 52 interiors; do not reuse MM7's 40-byte delta header or per-face/per-sprite attribute arrays. |
| Events | The offline parser audit finds at least 318 deterministic record failures across opcodes 12, 19, 20, 24, 26, 27 and 28. MM6 opcode 12 is a two-field bitmap operation in all 12 active records, while the inherited parser treats it as `EVENT_ShowMovie`. | Implement MM6-specific parsing and semantics, then cover every active opcode and event variable. Do not equate parse coverage with campaign correctness. |
| Event variables | 4,285 active compare/add/subtract/set records use 127 IDs. MM6's 100 map-byte IDs begin at 105, while the inherited MM7 map-variable range begins at 123 and has only 75 entries. MM6 skill, condition, resistance and party-state IDs also collide with different inherited names; `EvtVariables.cpp` has no title branch. | Decode and execute MM6 variables through a separate table and storage/save layout; verify event effects and persistence across the campaign. |
| Event text | Seven MM6 mouseover IDs exceed their `.str` count without a house-name override. Opcode 5 has 56 uses with an unverified operand; `EVENT_InputString` has 19 uses but its interpreter path currently asserts. | Resolve MM6 hint, location and input-dialog behavior against shipped scripts; keep all map text accessible. |
| Houses and media | MM6 uses 116 room animation IDs, but none of the corresponding MM7 `pAnimatedRooms` video names appears in the MM6 video indexes; seven `dsounds.bin` names are absent from the sound archive index. All 12 map music IDs have MP3 files. | Implement MM6 room descriptors, reconcile sound aliases/missing names, identify movie triggers, and verify house and map playback. |
| Character UI | `UIPartyCreation.cpp` allocates nine class icons and 22 portraits and draws MM7 classes including Monk and Thief | Build MM6 custom party and Quick Start paths, portraits, six classes and skills |
| Save/load | `SaveLoad.cpp` always serializes `SaveGame_MM7` and `IndoorDelta_MM7` / `OutdoorDelta_MM7`; file names and the `UISaveLoad.cpp` list filter require `.mm7` | Define, write, list and reload versioned MM6 saves and map deltas in the MM6 profile; leave MM7 saves readable in the MM7 profile |
| Launcher | Installed Atlas has only MM7 Play | Stage independent `MM6.app` beside verified `MM7.app`; preserve MM7 on install |

This table is a source-derived gap register, not a claim that these are the only
differences. Continue cross-file review and update it when evidence changes.

- [ ] Startup, title, credits, options, UI scale, input and intro videos. The
  MM6 reference card describes a New Game choice between custom party and
  pregenerated Quick Start; both paths need their own UI and initial state.
- [ ] Four-character party creation, portrait/voice selection, classes,
  statistics, starting skills, equipment and initial New Sorpigal state. The
  reference card specifies 50 starting stat points, two fixed and two selected
  skills per character, and a starting letter; verify the exact data mapping.
- [ ] Leveling, training, class promotions, skill points, mastery trainers,
  reputation, fame, conditions, aging, death and recovery.
- [ ] Equipment slots, armor class, weapon damage, identification, repair,
  enchanting, artifacts, quest items, shops and item generation. Include
  ingredient-to-potion and potion-to-potion mixing, failed mixtures, colored
  and white potions, and black potion side effects, as described in the
  [New World Computing mini guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf);
  reconcile recipes and effects with the shipped item/event data.
- [ ] Every spell ID 1–99 with Normal, Expert and Master behavior: target rules,
  mana, recovery, damage, resistance, status, buffs and world effects.
- [ ] Real-time and turn-based combat, projectiles, monster AI, pathfinding,
  loot, special attacks and resistances.
- [ ] Outdoor and indoor loading, transitions, weather, time, rest, respawn,
  traps, chests, doors, switches and map reset rules.
- [ ] EVT interpreter coverage for all shipped scripts; NPC dialogue, houses,
  guilds, quests, awards, travel, hirelings and endings.
- [ ] Scripted dungeon checks for switches, traps, keys, doors, teleports,
  encounters and quest rewards. Start with the Temple of Baa, Silver Helm
  outpost, Shadow Guild outpost and Dragoons' caverns described in the
  [official mini guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf).
- [ ] Audio, music, movie playback and settings on Apple Silicon.
- [ ] Separate `.ec6` save writer and reader: all party, inventory, quest,
  NPC, world, map delta, time and configuration state; atomic writes, versioning,
  backup and reload after exit.
- [ ] Private MM6 portrait and armor override hooks, each with Vanilla bypass.
- [ ] Offline validation for every table row, map and event resource, plus
  scenario checks for critical campaign branches and an ending path.
- [ ] Live visual and play validation from New Sorpigal through ending, with
  save/quit/relaunch/load at multiple checkpoints.

## Campaign coverage from the Prima guide

The [official strategy guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf)
has separate chapters for party creation, monsters, nine spell schools, items,
skills, NPCs, shops/locations, essential quests, promotion quests, minor quests
and maps. These headings are a coverage index; exact behavior remains subject
to the owned GOG tables and scripts.

- [ ] Character classes and party compositions: Knight, Cleric, Sorcerer,
  Paladin, Archer and Druid, including both promotion tiers of each.
- [ ] Monster families and their variant stats, encounter placement, loot,
  AI, special attacks and resistances.
- [ ] Spell schools: Fire, Air, Water, Earth, Spirit, Mind, Body, Light and Dark;
  all eleven spells per school and all three mastery levels.
- [ ] Item categories: weapons, body protection, wands, herbs, potions,
  scrolls, spellbooks, artifacts, relics, quest objects and miscellaneous items.
- [ ] Item generation and alchemy rules, including town-shop stock and treasure.
- [ ] Skill trainers, Expert and Master gates, guild access and class limits.
- [ ] NPC hiring, service effects, reputation response and quest dialogue.
- [ ] Weapon, armor, magic and general shops; stables, ship routes, temples,
  training grounds, guilds, taverns, banks, castles and dungeon entrances.
- [ ] Essential campaign chain: High Council, Nicolai, traitor, Oracle,
  Tomb of Varn, Control Center, Archibald and Hive, including the ending.
- [ ] All twelve class promotion quests, plus Seer and the minor regional
  quests across the listed outdoor map grid.
- [ ] Every indoor and outdoor map in the guide's appendix linked to its
  `mapstats.txt` ID, archive resources, events, quest requirements and exits.

## Trilogy coexistence contract

- [x] Three source worktrees exist side by side: MM6, MM7 and reserved MM8.
- [ ] All three are independently built as their own title-specific engines.
- [ ] Three sibling bundles inside The Atlas: `MM6.app`, `MM7.app`, `MM8.app`.
- [ ] Three independent game-data roots, profiles, save formats, mod folders,
  preferences, caches and logs. A title may read only its own runtime data.
- [ ] Rebuilding or installing one title preserves the other two app bundles,
  their saved games and their Atlas launch actions.
- [ ] Replace the MM7-era `launcher/build.sh` flow with a title-aware Atlas
  assembler. It must identify every input engine, stage existing playable
  siblings, reject a title mismatch, and leave the installed app intact on
  failure. In particular, an MM6 or MM8 build must never occupy MM7's helper
  slot.
- [ ] From one installed Atlas, launch all three campaigns in succession without
  rebuilding or swapping files; save and reload each, update one engine, then
  repeat all three checks.
- [ ] The Atlas marks each campaign playable only after its own verified
  graphical launch, campaign progression and save/load checks.
- [ ] Transfer directions unlock only when both corresponding engines and
  import/export adapters pass independent checks.
