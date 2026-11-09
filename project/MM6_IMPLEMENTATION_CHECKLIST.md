# MM6 implementation checklist

Status: **F4 in progress, engine table and merged-slice register reconciled
with source on 2026-11-07** (integration `ef8f0a3ee`). This is the public, asset-free
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
[monster/combat rules](MM6_MONSTER_COMBAT_SPEC.md),
[combat semantics](MM6_COMBAT_SEMANTICS_F2.md), and
[guide F2 review](MM6_GUIDE_F2_REVIEW.md). These are audit inputs, not
claims that their runtime behavior is implemented.

The [F3 source-data gate](MM6_F3_GATE_REVIEW.md) is accepted. Current F4 code
has a separate arm64 MM6 app target, original-order Games LOD transport lookup,
all 173 monster descriptors, title-specific vanilla portrait loading, a
bounded `.ec6` container envelope and complete active EVT **framing**. The
[F4 event packet](MM6_F4_EVENT_FRAMING.md) explains why parse success is not
event-effect or campaign success. Runtime rules, state serialization and F5
graphical play remain open.

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
- [x] Read and classify all 61 owner-manual pages and five reference-card pages
  at F2 page level in the [full manual/card review](MM6_MANUAL_CARD_FULL_REVIEW_F2.md).
  Gameplay prose pages were read with retained text and page images; framing
  story, illustrations and blank pages were classified separately. The review
  adds card-only `Y` to yell and item/target shortcuts, and flags contradictory
  sword/dagger mastery prose and incomplete spell/XP examples. Exact original
  rules and live controls remain F3/F5 questions. Compare the [original manual](https://www.mocagh.org/nwc/mm6-manual.pdf),
  [New World Computing guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf),
  [Prima guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) where accessible,
  and [MMExtension reference](https://grayface.github.io/mm/ext/ref/) when
  adjudicating source claims.
- [x] Review all 11 PDF pages of the [New World Computing limited-edition guide](MM6_NWC_GUIDE_FULL_REVIEW_F2.md)
  at page level. Its 14 named numeric potion results match retained matrix
  tokens, but guide-only side effects remain unknown; three of four dungeon
  headings have exact map-name joins and one has an alias question. Its 20-skill
  chart supports the manual's Sword/Dagger mastery table against a conflicting
  manual example, but omits eleven skills and does not prove executable
  effects. Exact claims and page evidence remain ignored locally.
- [x] Index the owned manual, reference card, Prima guide and short NWC guide
  by mechanic and campaign milestone in
  [MM6_GUIDE_RECONCILIATION.md](MM6_GUIDE_RECONCILIATION.md). Page locators
  and acceptance scenarios are recorded; detailed rule-by-rule comparison
  with shipped tables and actual effects remains open.
- [x] Index every page of the retained manual (61), reference card (5), and
  Prima guide (412) in an ignored fingerprint report and record 14 guide
  families, engine consumers, confidence and F3/F4/F5 scenarios in
  [MM6_GUIDE_F2_REVIEW.md](MM6_GUIDE_F2_REVIEW.md). Forty low-text pages
  need visual care; the index is not a sentence-by-sentence semantic review.
- [ ] Reconcile conflicting guide claims against the shipped GOG data. Keep
  guide citations and disputed claims here; never publish copied guide text.
- [x] Complete the single deliberate read-only original-file F3 preflight.
  F2 is frozen and the original **23/23** core/soundtrack identity check,
  bounded archive/media snapshot, and candidate PE extraction passed. The
  [content reconciliation](MM6_F3_CONTENT_RECONCILIATION.md) verifies 348
  targeted table/BIN/font/map/event members byte for byte; the
  [media reconciliation](MM6_F3_MEDIA_RECONCILIATION.md) verifies original
  VID/SND framing and soundtrack streams; the
  [PE reconciliation](MM6_F3_PE_RECONCILIATION.md) records creation, spell,
  house and travel data with named conflicts. Exact rows stay ignored.
  The [F3 matrix](MM6_F3_PREFLIGHT_MATRIX.md) family signoff and
  [gate review](MM6_F3_GATE_REVIEW.md) name the unresolved behavior and F4/F5
  checks. The [executable](MM6_F3_EXECUTABLE_FOLLOWUP.md),
  [portrait/voice](MM6_F3_PORTRAIT_VOICE_RECONCILIATION.md) and
  [actor-ID](MM6_F3_ACTOR_ID_RECONCILIATION.md) supplements tighten active
  code and asset requirements. F4 is authorized offline; none of these checks
  establishes a playable MM6.

## Shipped table coverage

Every source row is indexed in the private catalog. The numbers below are raw
nonempty rows; headers, separators and reserved records still require review.

- [x] Compare the 30 retained table names with literal C++ `eventsData` loads.
  `scripts/mm6_probe_table_consumers.py` records direct source-path/line hits
  and source fingerprints only in an ignored report. Twenty-four names have
  direct loads; six have none. [MM6_TABLE_CONSUMER_SPEC.md](MM6_TABLE_CONSUMER_SPEC.md)
  identifies the groups and open questions. This is not evidence that a
  loader understands every MM6 row or that an unreferenced table is unused.
- [x] Register each of the 30 retained tables with its immediate parser and
  at least one current downstream consumer or an explicit missing path in the
  [table semantic matrix](MM6_TABLE_SEMANTIC_F2.md). The reproducible
  `scripts/mm6_probe_table_semantics.py` records all table shapes, loader
  sites and ten selected numeric joins in an ignored report. All 367 positive
  NPC-to-house, 332 NPC-to-profession, 298 NPC-to-topic and 41 house-exit-map
  references resolve; these checks do not establish correct row meaning or
  reachability. One scroll row lacks an item template but lies outside the
  MM6 message-scroll loader range, so it is not a proven live missing item.
- [x] Identify the awards parser's total-drop mismatch. All 100 numeric MM6
  `awards.txt` rows have an empty third priority field; the shared
  `initializeAwards` skips a row when that field is empty, so it would skip
  all 100 in the current MM6 initialization path. The table semantic probe
  reproduces this source/data finding; it is not a runtime award check.
- [x] Load all 100 retained MM6 award rows without inventing a shipped
  priority. The F4 title-specific loader validates complete IDs and preserves
  MM7's parser; seven formatted rows remain guarded in the MM6 pane.
- [ ] Define MM6 award ordering and the seven formatted counters; verify
  display, event rewards and save/reload on representative promotion/quest
  routes while retaining MM7's award behavior.
- [ ] Give every table a verified parser/schema, title-specific consumer,
  active/reserved/unused classification, and acceptance scenario. Resolve
  indirect consumers or documented non-use for the six without literal loads
  before the consolidated F3 preflight.
- [x] Classify the six tables without literal C++ loads at F2 source level in
  [six-table role packet](MM6_UNLOADED_TABLE_ROLES_F2.md). Pinned MMExtension
  identifies `npcbtb.txt` as the 13-personality beg/bribe/threat matrix and
  `proftext.txt` as seven weekday profession-news topic/text pairs; the retained
  `useitems.txt` is a 29×29 combination matrix. `passwords.txt`, plural
  `autonotes.txt` and `errorlog.txt` have named provenance/consumer questions.
  The packet gives F3/F4/F5 acceptance and does not mark original use proven.
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
  live mixes in F4/F5. Also resolve the MM6 hired-NPC dismissal path and
  the `npcbtb.txt`, `passwords.txt` and autonote-variant roles.
- [x] Compare the owned guide's visually checked safe-potion chart with the
  retained matrix. `scripts/mm6_probe_potion_guide.py` confirms the same 25
  unordered input pairs, but two result rows disagree with shipped data.
  Exact recipes and guide transcription stay in ignored local reports; the
  [rules closure packet](MM6_RULES_F2_CLOSURE.md) records the evidence limit.
  Resolve both at F3 before encoding results or failure effects.

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
| `trans.txt` | 234 | Transition display text; the loaded C++ parser reads descriptions, not transport route schedules. MM6 stable/boat route source remains to identify. |
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
- [x] Maintain a private per-ID guide review queue for all 581 items and 173
  monsters in [MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md](MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md).
  OCR catalog matches are labeled separately from direct scans: 460 item
  entries and 39 monster IDs now have direct visual identity checks in that
  queue. Among items, 403 selected field sets match and 12 disagree with
  retained data; 164 item semantic rows remain unreviewed. These checks do
  not verify executable item effects or generation rules.
- [x] Give all 173 monster IDs an explicit catalog identity evidence state in
  [MM6_MONSTER_GUIDE_SEMANTICS_F2.md](MM6_MONSTER_GUIDE_SEMANTICS_F2.md).
  All 171 printed-catalog IDs now have direct page-image identity and selected
  field evidence; two tail IDs fall outside the guide's 57-family catalog.
  For 132 variants in the new packet, 50 of 528 level/HP/AC/XP cells and four
  of 132 treasure strings disagree with retained data; the prior 39 variants
  have a differently scoped selected-field set. Full attacks, effects,
  resistance, sprite/spawn joins and original combat remain open.
- [x] Join all 173 retained monster rows to inherited combat consumers and
  record identity, attack, missile, spell, special, resistance, AI and loot
  gaps with confidence and F4/F5 scenarios in
  [MM6_COMBAT_SEMANTICS_F2.md](MM6_COMBAT_SEMANTICS_F2.md). The ignored
  `combat-semantics-probe.json` preserves per-ID evidence. Exact Magic/Energy
  resistance, AI/factions, sprite/spawn joins, fixed drops and conditions
  remain open; this is not a working encounter.
- [x] Join retained monster descriptions, animation names and baseline map
  actors in [MM6_MONSTER_SPAWN_LINKS_F2.md](MM6_MONSTER_SPAWN_LINKS_F2.md).
  The ignored `monster-spawn-links-probe.json` holds exact joins. All 173
  descriptions match retained text-row picture tokens after normalization;
  1,382 of 1,384 animation references match the extended sprite-name index.
  A candidate ID 173 occurs among 342 baseline actors, while the current MM6
  loader discards description 173. The actor-ID offset is an external schema
  hypothesis, so F3 must confirm it before the F4 title-specific fix. No
  texture decode, live sprite or combat result is claimed.
- [x] Join all 173 retained monster rows to attack-preference, special and
  loot source paths in [MM6_MONSTER_EFFECT_SEAMS_F2.md](MM6_MONSTER_EFFECT_SEAMS_F2.md).
  Six guide-checked male-target preference rows reach an inherited Monk
  predicate; 26 numeric multi-target preferences are parsed but have no
  runtime reader. Three weapon-break rows reach an inverted equipped-weapon
  guard, and 12 inherited fixed-drop family rows would use numeric IDs in
  MM6's spell-scroll band. Exact mappings are private. These are static
  mismatches, not observed original or current MM6 encounter outcomes.
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
- [x] Audit every MM6 spell ID against static `pSpellDatas`, current cast
  cases, all spell scroll/book item bands, active event casts and monster spell
  labels. `scripts/mm6_probe_spell_effects.py` writes per-ID evidence privately;
  the [spell-effect F2 audit](MM6_SPELL_EFFECT_F2.md) records source paths and
  school-level acceptance. The MM6 text loader never replaces static MM7
  `pSpellDatas`; nine inherited rows require Grandmaster, all 99 MM6 scroll
  and 99 book IDs fall outside their corresponding inherited mapping ranges,
  and 22 of 129 active event-cast records reference IDs whose same-ID labels
  differ or are absent. The four inherited inventory-item target IDs also
  have different same-ID MM6 labels, so their MM6 target kinds need explicit
  reconciliation. None proves a reachable cast or correct effect.
- [x] Visually inspect all 56 Prima spell-section PDF pages and reconcile
  each of 99 guide base costs and all 297 Normal/Expert/Master clauses with
  the retained MM6 spell row in
  [MM6_SPELL_GUIDE_CLAIMS_F2.md](MM6_SPELL_GUIDE_CLAIMS_F2.md). All 99 base
  costs match; 22 differ from inherited static MM7 base cost. The private
  matrix has 207 quantitative, 87 qualitative and three guide-unknown rank
  clauses, and four guide/table rank-effect disagreements. Its same-ID C++
  screen finds 58 label/base-mana conflicts and 41 not-demonstrated effects.
  A later six-facet source screen covers all 99 IDs: 70 effect conflicts,
  27 limited candidates and two unresolved UI continuations; recovery is
  unresolved for 96. Full downstream behavior and runtime casts remain unverified.
- [x] Register [primary-source spell recovery leads](MM6_SPELL_RECOVERY_F2.md)
  for all 99 spells without inventing exact delays. The guide supplies a
  baseline category for every ID, explicit Expert/Master categories in 84
  cells and two numeric rank cells. MMExtension identifies three hard-coded
  delay fields per MM6 spell but publishes no values: zero complete original
  triples are verified, and all 96 previously unresolved recovery facets
  remain open. F3 must read all triples once from the original executable;
  F4 must implement a title-specific table and dynamic rules.
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
- [x] Give all 404 wand, scroll, book and 400–580 special-band IDs a retained
  role and current C++ consumer in the private queue described by
  [MM6 item/monster guide claims](MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md).
  Sixteen of 25 wand spell tokens disagree with inherited lookup. All 99
  scrolls, 99 books, 30 artifacts/relics and 50 message scrolls miss inherited
  type ranges, and 98 special-band IDs have held-item event operands. Sixty-seven
  special/misc roles and 164 item guide semantics remain unresolved; static
  consumer coverage does not establish their effects.
- [x] Triage the 164 unresolved item-semantic rows and current action paths in
  [MM6_ITEM_EFFECT_SEAMS_F2.md](MM6_ITEM_EFFECT_SEAMS_F2.md).
  `scripts/mm6_probe_item_effect_seams.py` keeps exact roles/events privately:
  66 broad special/misc rows, 50 message scrolls, 29 quest-described misc,
  15 outside the prior high-risk band, two quest-described equipment and one
  each spell scroll/book. All seven current direct-use item constants exceed
  MM6's max ID, and none of the 50 MM6 message scrolls passes the inherited
  read-window numeric guard. These are current-source mismatches, not verified
  original effects or a witnessed UI result.
- [x] Review late item-guide entries and retained held-item operands in
  [MM6_ITEM_GUIDE_CLOSURE_F2.md](MM6_ITEM_GUIDE_CLOSURE_F2.md).
  Of 43 individually printed late entries, eight give qualitative role clues
  and 35 give no explicit rule. A separate gem table matches nine of ten
  retained name/value pairs and disagrees on one identity. Fifty of 67 broad
  special/misc IDs have 400 held-item event references across 13 EVT files;
  the opcode signatures are grouped privately. Guide identity and script
  operands do not verify executable effects, so the 164 unresolved item
  semantics remain open for F3/F4/F5.
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
- [x] Join inherited new-game item/spell/class literals to retained MM6
  categories and the owned creation instructions. The reproducible
  `scripts/mm6_probe_creation_rules.py` and [creation F2 packet](MM6_CREATION_F2.md)
  find seven inherited starter spellbook literals outside the MM6 book band,
  an inherited bottle ID that is an MM6 spell scroll, two of seven spell
  unlock labels that differ or are absent at the same MM6 ID, nine MM7 class
  buttons, a Thief default and no MM6 Quick Start route. This is a source and
  retained-index mismatch, not an observed MM6 start or the correct fixture.
- [x] Record [pinned MM6 creation-constant source leads](MM6_CREATION_CONSTANTS_F2.md)
  for six family HP/SP bases, 18 promoted factors, 6×7 starting stats, 6×31
  starting-skill codes and 13 skill-recovery entries. The source provides
  process addresses and shapes, not values verified from the owner's
  executable. Retained MM6 Cleric class ID 3 reaches an inherited Black
  Knight no-mana path; this is a source/ID collision, not observed HP/SP.
  The guarded F3 PE-table helper has only passed synthetic tests and its
  open-gate refusal; it has not read the original executable.
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
- [x] Register whole-save logical field families in
  [MM6_SAVE_FIELD_REGISTER_F2.md](MM6_SAVE_FIELD_REGISTER_F2.md): container and
  slot metadata; party pose, time and resources; four characters; conditions,
  skills, spells, inventory and equipment; NPC/quest/award/note state; all
  67 map deltas; overlays, beacons, thumbnail and active selection. Each family
  has a current MM7 source path and MM6 persistence scenario. Exact `.ec6`
  fields and MM6 meanings remain to be locked after F3.
- [x] Audit retained MM6 delta records against a pinned primary editor reader
  and current C++ snapshots using `scripts/mm6_probe_world_fields.py`.
  [MM6_WORLD_FIELD_MAP_F2.md](MM6_WORLD_FIELD_MAP_F2.md) records 342 actor,
  348 ground-object, 1,340 chest and 10,400 fixed door records, with an
  ignored per-map report. All 1,340 baseline chests have zero grid bytes.
  Later [original placement and save-path traces](MM6_F4_CHEST_GRID_WIRE.md)
  establish the full final 280 bytes as 140 signed grid cells in memory and
  serialized delta output; live grid projection and changed saves remain gated.
- [x] Reassemble all 67 retained MM6 deltas byte-exactly from bounded wire
  sections using `scripts/mm6_probe_world_roundtrip.py`. Its ignored report
  records 268 synthetic sentinel checks across fixed regions and 268 malformed
  input rejections. This proves Python section preservation only; all shipped
  reset headers, persistent blocks and tails are zero, so nonzero field
  meanings and C++ save behavior remain open. The [world closure packet](MM6_WORLD_F2_CLOSURE.md)
  maps those regions to current consumers and acceptance checks.
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
  and game behavior before implementing them. Update 2026-11-07: the enum now
  carries `EVENT_MM6_Opcode20`, `27` and `28`, and the interpreter advances
  past them without effect as the original dispatcher does
  (`MM6OriginalNoopEvent.AdvancesWithoutEffect`). Other operands stay open.
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
- [x] Register all 43 active MM6 opcode values with retained payload widths,
  current parser/executor case presence, source-led interpretation confidence
  and an F4 scenario in [MM6_OPCODE_SEMANTICS_F2.md](MM6_OPCODE_SEMANTICS_F2.md).
  `scripts/mm6_probe_opcode_semantics.py` verifies 68 active files and 14,477
  frames, and keeps exact operands/locations in an ignored report. The source
  audit confirms no inherited cases for opcodes 20, 27 or 28; candidate
  meanings and all title-specific side effects remain unverified.
- [x] Map each of the 43 active opcodes to a candidate state target, current
  control-flow route, persistence obligation and F4/F5 scenario in
  [MM6_OPCODE_SIDE_EFFECTS_F2.md](MM6_OPCODE_SIDE_EFFECTS_F2.md). The seven
  high-conflict values retain explicit unknown original operands or effects;
  this is a source-backed contract, not an implemented interpreter.
- [x] Audit the complete event-load gate and a retained skill-check branch in
  [MM6_EVENT_EXECUTION_SEAMS_F2.md](MM6_EVENT_EXECUTION_SEAMS_F2.md).
  `scripts/mm6_probe_event_execution_seams.py` confirms 318 current parser
  rejections in 26 of 67 listed map EVT files; those files contain 6,964
  active records and 1,783 variable operands. `EvtProgram::load` rejects the
  whole resource on its first bad record, so all-resource parse is the first
  F4 event milestone. A later opcode-43 branch cannot succeed under the
  current exact mastery comparison; F3 must confirm the MM6 zero-mastery
  meaning before F4 changes it. No map-load crash or quest failure was
  observed in a running MM6 game.
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
- [x] Join house, travel, NPC-topic and ending references against current
  consumers in `scripts/mm6_probe_campaign_routes.py`. Its ignored report
  records 21 typed transport houses, six stable IDs outside the inherited
  route-array range, 74 NPC topic references with global functions that the
  inherited teacher range intercepts first, and two Hive terminal entries
  routed to the MM7 game-over path. The [campaign closure matrix](MM6_CAMPAIGN_F2_CLOSURE.md)
  records source paths and acceptance scenarios; none of these joins proves
  original behavior or campaign reachability.
- [x] Inventory the 21 transport-house A/B/C records in retained `2devents.txt`.
  [MM6_TRANSPORT_FIELDS_F2.md](MM6_TRANSPORT_FIELDS_F2.md) and the tracked
  `scripts/mm6_probe_transport_fields.py` find 37 populated cells: 34 regular
  schedule-shaped candidates, one unknown day-token candidate and two special
  forms. Exact cells are in an ignored report. The inherited house loader
  drops these route meanings; the original schedule, cost and destinations
  remain unproven.
- [ ] Implement MM6-specific transport schedules/destinations and NPC-topic
  precedence after reconciling the retained scripts and guide. Give the Hive
  terminal state its own ending, certificate/video and post-finale policy;
  the current MM7 handler writes `MM7_Win.Pcx` and respawns at Harmondale.
- [x] Build a private structural graph of active event functions, map moves,
  house exits, quest-bit operands and NPC topic changes. The reproducible
  `scripts/mm6_probe_quest_graph.py` records detailed links only in ignored
  `quest-graph-probe.json`. All 93 explicit script map targets and 41 house
  exit targets resolve to listed MM6 maps; four quest bits are read without a
  script write and 70 are written without a script read. These are structural
  leads, not gameplay defects or proof of ending reachability.
- [x] Give all 32 Prima campaign headings/groups private candidate quest,
  event, NPC, item and map joins, plus acceptance checks in
  [MM6_CAMPAIGN_GUIDE_CLAIMS_F2.md](MM6_CAMPAIGN_GUIDE_CLAIMS_F2.md).
  Twenty-eight have a named note and structural EVT lead; all 12 promotions
  have distinct note leads. All 36 printed transport options were compared
  with retained table cells: two durations differ and one extra boat lacks a
  printed itinerary. These are candidates, not branch or travel proof.
- [ ] Resolve the graph's guide milestone candidates against actual EVT
  conditions, item/NPC/house effects, alternate routes and persistent state;
  demonstrate a New Sorpigal-to-ending route and required promotions with
  scenario checks before F4 is considered complete.
- [x] Inventory the one-byte variable IDs in all active MM6 compare/add/
  subtract/set events. The reproducible `scripts/mm6_probe_event_variables.py`
  records 4,285 references to 127 distinct IDs in the ignored
  `event-variable-probe.json`, with event offsets, values, source hashes, and
  both MM6 and inherited enum labels. Of these references, 1,609 use IDs 46
  or above where MM6's variable vocabulary cannot be assumed to match MM7's;
  many, but not all, collide. The format-2 report now fingerprints all 68
  active EVT inputs and the inherited enum, interpreter and variable consumer.
  Of those references, 1,240 are
  MM6 map bytes 1–100, 222 are MM6 party-state/autonote variables, 54 are
  conditions, 40 are skills, and 53 are MM6's resistance taxonomy. Concrete
  collisions include MM6 ID 105 (map byte 1) versus inherited `VAR_Cursed`,
  and MM6 ID 205 (event autonote) versus inherited decoration variable 7.
  The [MM6 editor's target-ID list](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java)
  supplies the vocabulary; the owned scripts establish which IDs are active.
  [MM6_VARIABLE_DISPATCH_F2.md](MM6_VARIABLE_DISPATCH_F2.md) records the
  title-specific source/consumer and F4/F5 acceptance contract.
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
- [x] Classify all nine unlisted `.str` files at retained-data and current-source
  level in the [extra-STR role packet](MM6_UNLISTED_STR_ROLES_F2.md). Five have
  bounded same-basename extra EVT files but no listed/base map; three are
  opening/win/loss narrative candidates and one is table-shaped. Every file
  has a private status and F3/F4/F5 route. No extra script is counted as an
  active listed-map EVT or a safe alias based only on byte/text similarity.
- [ ] Resolve the seven direct mouseover exceptions, opcode 5 semantics, and
  original use of the nine unlisted `.str` files, including `intro`, `win` and
  `lose`, then verify any reachable routes in F4/F5.
- [x] Validate record boundaries of all 11 shipped `.bin` resources. Ten use
  a four-byte count followed by fixed-size records; `dsft.bin` has two counts,
  6,455 sprite frames and 1,656 extended-frame indices. The ignored
  `binary-table-probe.json` records counts, lengths and hashes. `dchest.bin`
  has eight 36-byte records; every MM6 row declares 14×10 chest layout metadata and
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
- [x] Record [primary-source MM6 media schema leads](MM6_MEDIA_SCHEMA_LEADS_F2.md)
  against retained F1 indexes. The pinned VID/SND readers use the same
  44/52-byte directory widths as current C++, but MM6 video names are
  extensionless in the retained 127-entry index while current lookup appends
  a suffix. Twelve complete 01–53 icon-frame groups (636 extracted PNG
  headers, 59×79) are strong vanilla portrait candidates; their identity and
  playback are not verified. F3 must check raw original headers, names,
  payload boundaries, codecs and frames before F4 locks loaders.
- [x] Register [MMExtension MM6 executable table locators](MM6_EXECUTABLE_TABLE_LOCATORS_F2.md)
  for 100 14-byte spell records, 119 16-byte house-movie records and 36
  candidate transport entries. The 116 distinct retained room IDs span
  1–118, within the candidate house table's 0–118 bounds. These are pinned
  process-memory source leads, not verified offsets or owner-executable data;
  F3 must map and validate them in the planned read-only pass.
- [x] Join retained media triggers to current consumers in the
  [media trigger map](MM6_MEDIA_TRIGGER_MAP_F2.md): all 67 map music references
  and eight active event-sound uses resolve to retained names/IDs. The 116
  distinct MM6 room IDs yield zero matching inherited MM7 movie names, and
  suffix-form movie lookup matches none of the 127 extensionless retained VID
  names. These are static joins, not correct cue assignments or playback.
  F3 must identify MM6 house/movie/effect mappings; F5 must see and hear them.
- [x] Compare inherited creation icons, portrait-frame construction, literal
  fonts and potential voice-reaction IDs to retained MM6 indexes using
  `scripts/mm6_probe_media_consumers.py`. The [media F2 closure packet](MM6_MEDIA_F2_CLOSURE.md)
  records source paths and acceptance checks. Five of nine inherited class
  icons match, none of 22 inherited first portrait-frame names matches, and
  all 12 literal font names match. Potential reaction sound IDs have many
  table gaps; these are source/index risks, not a verified original voice map.
- [x] Inventory every named current screen ID and its source consumer in the
  [MM6 UI screen audit](MM6_UI_SCREEN_AUDIT_F2.md). This source register does
  not establish an MM6 layout. It names title-specific acceptance for menu,
  play, character, books, dialogue, rest, chest, house, spell, save/load and
  ending screens, and flags the hard-coded MM7 decoration-topic base and
  inherited paperdoll artifact assertions.
- [x] Reconcile the owner's manual, reference card and service guide chapter
  at page-locator level in [MM6 manual/services claims](MM6_MANUAL_SERVICES_CLAIMS_F2.md).
  The first source-level service collision is MM6 training house IDs 79–88
  indexing an MM7 cap table for 89–98; temple restrictions and NPC profession
  effects also need MM6-specific joins. No live service behavior is proven.
- [x] Join all 77 MM6 profession rows to guide names, NPC references and
  inherited C++ effect paths in [MM6 NPC profession packet](MM6_NPC_PROFESSION_F2.md).
  Guide professions 1–51 match retained names; 26 generic rows remain
  guide-unlisted, 20 generic IDs have `canJoin` flags in NPC data, and seven
  generic IDs alias MM7 special effects. Hireability and original effect
  numbers are still uncertain; the exact per-ID ledger remains ignored.
- [x] Trace initial house and topic routes for all 70 NPC records using the
  26 generic profession IDs in [MM6 NPC profession packet](MM6_NPC_PROFESSION_F2.md).
  Forty have a static initial paid-hire choice, 23 have a house-conversation
  route without that choice, and seven need outdoor/event routing. Dynamic
  house/topic changes and actual hireability remain unproven.
- [ ] Video and audio archive structure, house/movie routing, playback and
  fallbacks. The 116 distinct MM6 house animation IDs used in `2devents.txt`
  are within the inherited 196-entry MM7 `pAnimatedRooms` array, but **zero**
  of their MM7 video names occur in the retained MM6 `Anims1`/`Anims2` indexes.
  Build an MM6-specific room descriptor table rather than indexing the MM7 one.
- [x] Join active MM6 house-entry events to the retained house table and the
  current parser's type labels. `scripts/mm6_probe_house_routes.py` records
  674 `SpeakInHouse` instructions, 521 distinct targets, 519 table-backed
  targets and two special terminal handlers in an ignored per-event report.
  Of the table-backed active targets, 384 had nonempty labels that the legacy
  parser silently routed to `HOUSE_TYPE_MERCENARY_GUILD`. The F4 title-specific
  parser now classifies all retained labels; this is not an observed runtime
  result or a resolved service mapping.
- [ ] Give every active MM6 house type and entry target an intentional UI,
  service, movie/background and exit policy. Remove inherited MM7 numeric
  range assumptions from `isShop`/`isMagicGuild` and test house entry,
  training, shops, travel and finale after the parser boundary.
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
  invokes movies outside these active map events. Update 2026-11-07: opcode 12
  now parses and runs as an MM6 outdoor texture operation
  (`EvtInstruction.cpp`, `EvtInterpreter.cpp`). Its visible effect and the
  movie triggers stay unverified.
- [ ] Compare one low, middle and high record of every ID family and every
  structure boundary with MM7. Record incompatible assumptions explicitly.
- [x] Record the retained MM6 class and skill IDs, source row widths, known
  inherited collisions and creation UI asset families in
  [MM6_CLASS_SKILL_SPEC.md](MM6_CLASS_SKILL_SPEC.md). MM6 has six three-tier
  class families (IDs 0–17) and 31 skills (IDs 0–30); inherited MM7 skill
  ID 30 is Dodge where MM6 has Learning. Affinity, HP/SP and portrait/voice
  semantics remain to be established and implemented.

## Engine and campaign behavior

The current MM6 branch is not a complete engine. The table below was
reconciled with source on 2026-11-07, after the F4b and F4d lanes merged. Each
row names what runs, the source or test that backs it, and what stays open:

The draft [F4 offline build graph](MM6_F4_BUILD_GRAPH.md) orders these seams by
dependency; it must be reconciled with the F3 original-file report before any
runtime implementation phase begins.

| Runtime area | Current evidence | Required MM6 work |
| --- | --- | --- |
| Startup and resources | `Engine.cpp` still enters `MM6_Initialize`, `MM7_LoadLods` and `MM7_Initialize` | Audit each initializer and make title-specific asset, layout and rule paths explicit |
| Items | `MM6ItemTable.cpp` reads MM6 tables but still populates shared special/artifact bonus maps. MM6 scrolls, books and messages occupy the inherited MM7 scroll, book and artifact ID bands. The full MM6 random item generator is traced from the original in [`MM6ItemGenerator.cpp`](../src/Engine/MM6/MM6ItemGenerator.cpp) (`MM6ItemGenerator_ut.cpp`). Chest any-item rolls and event opcode 41 use it. | Replace the remaining item-kind, spell mapping and bonus assumptions. MM6 special-bonus effects in the runtime, potion recipes and item use stay open. Verify every MM6 item ID and equipment rule. |
| Inventory and equipment | MM6 paperdoll rules from the original executable live in [`MM6PaperdollEquip.cpp`](../src/Engine/MM6/MM6PaperdollEquip.cpp), dispatched from `UICharacter.cpp`. Inventory capture writes each item's raw 28-byte record at its current record and slot, so moves, arrivals and removals save ([bounded inventory packet](MM6_F4_BOUNDED_INVENTORY_PROJECTION.md)). `Party::setHoldingItem` and `takeHoldingItem` keep the raw HELD owner in step. Backed by the gate child `MM6V5PaperdollHeadless.DISABLED_BackpackItemStaysEquippedThroughInventoryCapture`. The F5a character screen now resolves MM6 base, armor, weapon, boot, helm, belt and cloak positions; thirteen focused character checks and a hidden cloak capture pass. The MM7-only right-panel border lookup was removed from MM6 and fourteen combined character/house checks passed. | Consume and sell removal are untraced; original screen alignment and other UI icon requests remain open. Tail bytes of reused records, backpack swap with an item in hand and auto-placement order stay open. Items with no raw record still refuse a save. |
| Spells | MM6 spell names differ from inherited numeric labels at 49 of 99 IDs under a strict lexical check. 27 IDs have different mana triples. [`MM6SpellEffectGuard.h`](../src/Engine/MM6/MM6SpellEffectGuard.h) admits player casts of ID 67 (Cure Weakness) from the spellbook and quick cast, and monster casts of IDs 2, 4, 18 and 24. Every other regular ID refuses, the spellbook with a status message. Scroll, NPC and temple casts refuse before queueing ([spell dispatch packet](MM6_F4_SPELL_EFFECT_DISPATCH.md)). | Implement MM6's own 99 spell definitions and effects across every school and mastery, including the linked MM6 scroll/book/wand mappings. First Aid, Power Cure, the other timed cures, Shield and Heroism are implemented but stay behind the guard. |
| Combat | Party, monster and monster-on-monster hits run an MM6 preflight before any state or RNG changes ([`MM6CombatRules.h`](../src/Engine/MM6/MM6CombatRules.h)). Characters take Electricity and Cold with the original halving rolls. Monster spells 2, 4, 18 and 24 fly as single projectiles with original monster damage ([`MM6MonsterSpellEffect.h`](../src/Engine/MM6/MM6MonsterSpellEffect.h)). The `BrkItem` special attack runs in an MM6 helper. Backed by three opt-in `DISABLED_` tests in `GameTests_MM6Combat.cpp`, each run alone. | Monster-side Electricity and Cold, Poison, Magic and Energy stay closed. Inherited monster AI, monster missiles, other special attacks, turn mode, the second-attack bonus and the MM6 portrait animation mapping stay open. The each-spell-element choice over vanilla's all-Fire is an owner decision. |
| Chests | MM6 `dchest.bin` declares 14×10 layout metadata, read through `mm6::chestTypeLayout` in `Chest.cpp`. The 4,204-byte MM6 delta chest record has 140 28-byte item slots followed by 140 signed grid cells. The first-map load attempt applies the saved chest owners at `_461103_load_level_sub`. Chest any-item rolls go through the MM6 generator (`MM6ChestCallerAdapter.h`). | Prove placement, opening and changed-state save/reload on maps other than the first, without changing MM7 chests. |
| Outdoor maps | `OutdoorLocation::Load` has an MM6 branch that decodes the base map with `deserializeMM6Outdoor` and the delta with `decodeMM6OutdoorDelta` (`Outdoor.cpp`). All 15 shipped pairs decode (`MM6V5OutdoorSourceHeadless.AllShippedOutdoorPairsDecode`). Only the first map is reachable in play. One later map, `oute2.odm`, has a direct world-load probe (`MM6V5LaterOutdoorHeadless.DISABLED_DirectSecondOutdoorLoadProbe`). | The 200 persistent bytes and tail still have no verified mapping. Prove geometry, collision, decoration, spawn, events and persistence across all 15 maps through real travel. |
| Indoor maps | `IndoorLocation::Load` has an MM6 branch that decodes `.blv` and `.dlv` through `MM6IndoorSnapshot` (`Indoor.cpp`). Indoor maps load only in test fixtures, the v6 indoor children, because no live transition reaches them. | Respawn and delta persistence through real indoor entry and exit for all 52 interiors. |
| Events | All 68 active EVT resources parse under the MM6 reader (`MM6EventFraming.AllActiveRetainedResources`, [framing packet](MM6_F4_EVENT_FRAMING.md)). Opcodes 12 and 24 are MM6 outdoor facet operations, 26 is an MM6 question, 20, 27 and 28 advance without effect (`MM6OriginalNoopEvent.AdvancesWithoutEffect`), 11 and 23 use the MM6 indoor face index, 39 and 40 go through `MM6NpcEventState`, and 41 uses `MM6GiveItemEvent.cpp`. | Opcode 34 (summon item) still runs the MM7 path. Opcode 21 casts stay behind the spell guard. Cover every remaining active opcode and do not equate parse coverage with campaign correctness. |
| Event variables | `describeMM6Variable` classifies all MM6 IDs ([`MM6EventVariables.h`](../src/Engine/Evt/MM6EventVariables.h)). MM6 dispatch files run quest bits, item in hand, gold, day of year, skills, map bytes 105-175, autonote, character bits, flying, month, resistances 46-55, NPC in party 214, day counters 216-221, hireling 223, circus prizes 224, skill points 225 and all condition writes 87-104 ([`MM6EventVariableDispatch.cpp`](../src/Engine/Evt/MM6EventVariableDispatch.cpp), `MM6EventVariables.InterpreterUsesMM6ResistanceSkillPointHireAndConditionStorage`). IDs 1-45 use the inherited executor behind MM6 value checks. Startup selects the MM6 year 1165. The first-map v5 save/load gate restores its represented event state. | Reputation 215 needs a party field and a save section. Map bytes above 175 refuse. Condition writes skip protection and health changes, which the original effect still has to confirm. Event persistence beyond the first-map v5 scope remains open. |
| Event text | Seven MM6 mouseover IDs exceed their `.str` count without a house-name override. Opcode 5 has 56 uses with an unverified operand. The 19 opcode-26 records now run as MM6 questions through `startMM6QuestionDialogue`. | Resolve MM6 hint and location behavior against shipped scripts, keep all map text accessible. |
| Houses and media | House entry runs through `mm6EnterHouse` and the MM6 house screen in [`MM6House.cpp`](../src/GUI/UI/Houses/MM6House.cpp), with room media from [`MM6RoomDescriptors.cpp`](../src/GUI/UI/MM6RoomDescriptors.cpp). Entry refuses before state changes when a table, manifest, icon or movie is missing. Resident NPC topics run. Temple/training actions now call the MM6 house sound dispatcher where appropriate. Seven `dsounds.bin` names remain absent from the sound archive index. | Opening hours, hiring topics, save/reload of house state and a visual check of the screen stay open. Reconcile sound aliases and identify movie triggers. |
| Character UI | `UIPartyCreation.cpp` uses the MM6 twelve-face, six-class creation route and title-specific starting skills/items. The character record has five MM6 tab/exit buttons; base figure, armor, weapon, boots, helm, belt and cloak art now have MM6 routes ([F5a runbook](MM6_RESUME_HANDOFF.md)). The focused character/house checks passed 14/14 and a hidden render found pixels in the button regions. | Production New Game saving remains guarded. Original tab and paperdoll alignment, other missing screen art and visible input/save acceptance remain open. Quick Start still needs its separate production gate. |
| Other UI screens | [Screen register](MM6_UI_SCREEN_AUDIT_F2.md) maps the inherited screens. The MM6 HUD has title-specific frame, books/actions hit boxes, portrait bars and animated selected frame. Four HUD books open and close through a headless first-map check; the common frame/exit tab and Quest page use native MM6 art and original positions ([HUD packet](MM6_F4_HUD_RESOURCES.md)). The paperdoll no longer enters inherited MM6 artifact assertions. Hidden world and character captures prove bounded rendering only. | Notes and Map still request inherited art. Reconcile their pages, dialogue, rest, hints, menus, options and ending visuals with title-specific data; inspect placement and input in F5a/F5b. |
| Services | The MM6 house screen lists proprietor services from `houseServiceOptions`. Temple healing and training dispatch through MM6-specific implementations ([`MM6House.cpp`](../src/GUI/UI/Houses/MM6House.cpp)); other listed services currently report unavailable. Seven generic MM6 NPC profession IDs still alias inherited MM7 effects. | Wire and verify the remaining shop, guild, bank, tavern and donation actions; check opening hours, merchant discounts, sound aliases and state persistence. |
| Transport | `Transport.cpp` has an MM6 branch and `selectTransportMenu` plans the routes, but the MM6 house screen never builds the transport window, so neither is reachable. | Build the MM6 transport menu and departure after the map transition wiring. |
| Save/load | The MM6 Save menu writes a numbered `saveNNN.ec6` slot through the native atomic writer after an admitted first-map Load. Production `loadGame()` calls `mm6::loadFirstMapManualSlot` for MM6; the Load menu lists only validated numbered first-map slots. Fresh child processes loaded, moved and saved again with backup rotation. The integrated first-map gate passed 25/25, including three MM7 controls ([resume handoff](MM6_RESUME_HANDOFF.md), [`SaveLoad.cpp`](../src/Engine/SaveLoad.cpp)). | Production New Game save, quicksave, quickload, autosave, in-game load, cross-map travel and saves beyond the first map remain guarded. F5a visible opening and input/save verification remain open. |
| Transitions | Title-aware `isMapIndoor` and `isMapOutdoor` now classify MM6 maps in world preparation (`MapEnumFunctions.h`, `Engine.cpp`). Detached pieces include the visited-map cache, visited indoor owner, `VIST` v2 session codec, border confirmation on a Party copy, `encodeMM6ArchiveEntry`, `restAndHealDetachedParty` and four border hirelings. The MM6 indoor transition dialog remains Cancel-only; `getTravelDestination` returns nothing for MM6, and `startMapTransition` still calls guarded `autoSave()`. | Wire the pieces per the [transition wiring plan](MM6_TRANSITION_WIRING_PLAN.md). Later-map and indoor live capture are missing. Two per-character fields the trip zeroes are unmapped and regeneration catch-up after the jump is unchecked. |
| Death and recovery | Party death and respawn are modelled detached in [`MM6PartyDeath.cpp`](../src/Engine/MM6/MM6PartyDeath.cpp) (`MM6PartyDeath_ut.cpp`). `Game.cpp` still respawns a dead MM6 party with the MM7 rule. | Wire the model into one `Game.cpp` dispatch after production load lands. |
| Launcher | Installed Atlas has only MM7 Play | Stage independent `MM6.app` beside verified `MM7.app`; preserve MM7 on install |

This table is a source-derived gap register, not a claim that these are the only
differences. Continue cross-file review and update it when evidence changes.
The Startup, Character UI, Other UI and Launcher rows were not rechecked on
2026-11-07.

### Merged F4b and F4d slices, 2026-11-07

Each checked item names the test that proves it. The lane reports and their
test counts are in [the session handoff](SESSION_HANDOFF_2026-11-07.md). Tests
marked `DISABLED_` are opt-in children that `tools/run_mm6_firstmap_gate.py`
or a single-test run enables, they do not run in the default suite.

- [x] The Engine first-map attempt owns EVRT, actors, generated NPCs,
  decorations, sprites, the chest handoff and RNG in native order, and checks
  the prepared world before first input. A refused check abandons to the main
  menu. Source: `MM6V5FirstMapLoadAttempt.{h,cpp}`, `Game.cpp`. Test: gate
  fault child `MM6V5FirstMapAbandonHeadless.DISABLED_PostWorldMismatchThenFullReloadAndInput`.
- [ ] A production route constructs the attempt from a numbered slot and
  publishes it through `loadGame()`. Only test code constructs it today (Lane L).
- [x] The normal Save menu writes a numbered `saveNNN.ec6` slot atomically and
  keeps the old slot byte-identical on a refused or failed write. Test:
  `MM6V5NormalSaveMenuHeadless.DISABLED_SaveMenuWritesNumberedFirstMapSlot`.
- [x] The Load menu lists only loadable numbered slots of the native MM6
  profile and hides malformed and foreign files. Test:
  `MM6V5NormalLoadMenuHeadless.DISABLED_LoadMenuListsAndLoadsNumberedFirstMapSlot`.
  That child loads the slot through test-only replay scopes, not through
  production `loadGame()`.
- [ ] Save is armed after a production load releases the attempt, and a
  New Game → save → fresh-process load → save again chain passes through the
  menus. This closes F4b (Lane L).
- [x] Event variables 46-55, 214, 216-221, 223-225 and condition writes 87-104
  run from MM6 dispatch. Test:
  `MM6EventVariables.InterpreterUsesMM6ResistanceSkillPointHireAndConditionStorage`
  and the unit tests under `src/Engine/Evt/Tests/`. Variable 215 stays refused.
- [x] Opcode 41 rolls through the MM6 item generator and moves a held item to
  the active character first. Test:
  `MM6GiveItemEvent.RollsOnMM6GeneratorAndMovesHeldItemToActiveCharacter`.
  Ground drop for a full party and save/reload of the result stay open.
- [x] MM6 houses open their own screen, list resident NPC topics and run the
  temple heal. Other services refuse. Tests: `MM6Houses.EnterAndExit`,
  `MM6Houses.UnimplementedServiceRefuses`, `MM6Houses.TempleHeal`,
  `MM6Houses.HouseNpcTopic`, `MM6Temple_ut.cpp`.
- [x] Backpack and paperdoll moves save with the MM6 equip rules. Test: gate
  child `MM6V5PaperdollHeadless.DISABLED_BackpackItemStaysEquippedThroughInventoryCapture`.
- [x] First-map combat runs to completion: monster spells 2, 4, 18 and 24 hit
  the party, the Goblin King's `BrkItem` breaks an item and Cure Weakness casts
  from the spellbook. Tests: `MM6CombatHeadless.DISABLED_FirstMapMonsterSpellsHitParty`,
  `MM6CombatHeadless.DISABLED_GoblinKingBreaksItem`,
  `MM6CombatHeadless.DISABLED_CureWeaknessFromSpellbook`.
- [x] The detached transition owners pass their unit tests: map type helpers,
  visited-map cache and `VIST` codec, visited indoor owner, border
  confirmation, archive encoder (`MM6Archive.ShippedDeltasRoundTrip`) and the
  Cancel-only indoor dialog (`MM6TransitionDialog_ut.cpp`). A border trip rests
  a Party copy and leaves the live party alone
  (`MM6BorderRest.ConfirmationRestsOnlyTheCopy`).
- [ ] Wire the transition owners into border travel, indoor entry and exit,
  and the save, per the [wiring plan](MM6_TRANSITION_WIRING_PLAN.md). Then
  enable the indoor dialog's Yes button.
- [x] The party death model passes detached (`MM6PartyDeath_ut.cpp`).
- [ ] Wire MM6 party death and respawn into `Game.cpp`.
- [ ] F5a staged graphical opening check. Hidden world and character captures
  pass, and a disposable first-map Load profile can be staged through five
  fresh-process checks. Visible opening, input and save/reload remain unverified;
  production New Game is guarded.

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
