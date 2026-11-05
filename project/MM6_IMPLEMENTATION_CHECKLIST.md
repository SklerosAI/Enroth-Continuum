# MM6 implementation checklist

Status: **inventory in progress, 2026-11-05**. This is the public, asset-free
index. The exhaustive per-file and per-row catalog is private at
`local-game-data/mm6/research-2026-11-05/catalog.json` and is ignored by Git.
An inventoried row is not an implemented game rule. Check an item only when a
title-specific implementation and an appropriate offline or live check exist.

## Evidence and boundaries

- [x] Preserve the MM7 source baseline on `mm7-isolated-runtime`; independent
  Atlas build launches its own `MM7.app` and loads the existing MM7 save.
- [x] Copy and hash the local MM6 LOD archives into an ignored reference set.
  Current archive lists contain 2,703 icon, 134 map, 1,958 bitmap and 4,314
  sprite entries. Extraction produced 9,128 files including manifests; image
  conversion and generated names mean extracted count is not an archive count.
- [x] Catalog every extracted file with size and SHA-256, plus every nonempty
  row of the 30 text tables, locally. This is raw coverage, not semantic review.
- [x] Extract text locally from the owned 61-page manual and 5-page reference
  card. Both source PDFs and text are ignored; their sections still need full
  mechanic-by-mechanic review.
- [x] Inventory the installed video, sound, music and executable file families:
  `Anims1.vid` has 40 entries, `Anims2.vid` has 87, `Audio.snd` has 1,526,
  and `Sounds/2.mp3` through `16.mp3` provide 15 music tracks. Archive and
  executable hashes, and per-track music hashes, are in the ignored research
  folder. The original executable is PE32 x86; it is reference material only.
- [ ] Decode media entry formats and map all movie, effect and music IDs to
  game behavior; inspect native MM6 save format if a lawful local sample exists.
- [ ] Read the owned original manual and reference card and annotate every
  relevant mechanic. Compare the [original manual](https://www.mocagh.org/nwc/mm6-manual.pdf),
  [New World Computing guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf),
  [Prima guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf) where accessible,
  and [MMExtension reference](https://grayface.github.io/mm/ext/ref/).
- [ ] Reconcile conflicting guide claims against the shipped GOG data. Keep
  guide citations and disputed claims here; never publish copied guide text.
- [ ] Run a single deliberate read-only original-archive preflight after this
  specification is complete. Record all counts, schema lengths, ID gaps,
  boundary samples and unresolved differences in the ignored local report.

## Shipped table coverage

Every source row is indexed in the private catalog. The numbers below are raw
nonempty rows; headers, separators and reserved records still require review.

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
| `class.txt` | 19 | Class tree, promotions, starting skills and equipment |
| `stats.txt` | 26 | Character statistics and derived values |
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

## Archive and map formats

- [ ] LOD directory, compressed payload and case-insensitive lookup semantics.
- [ ] Icon, bitmap and sprite formats, palettes, transparency, animation frames,
  equipment positions, character faces and title-specific UI resources.
- [x] Verify the 67 map filenames in `mapstats.txt` match the 67 base files in
  `Games.lod`, with every 15 outdoor and 52 indoor file having its delta pair.
  All 134 retained map entries have valid size headers and their raw DEFLATE
  streams produce exactly the declared sizes. Strict zlib validation fails
  Adler-32 on `cd3.blv` and `d08.blv`; the exact cause and safe decoding policy
  remain open. The ignored `map-probe.json` records entry and decoded hashes.
- [ ] All 15 `.odm` outdoor maps and 15 `.ddm` delta companions: geometry,
  terrain, objects, actors, doors, chests, map events and persistence.
- [ ] All 52 `.blv` indoor maps and 52 `.dlv` delta companions: geometry,
  portals, rooms, lights, objects, actors, doors, chests and persistence.
- [x] Frame all 67 listed map EVT files and `global.evt`: 14,477 records, 43
  distinct opcodes, no framing errors. The other 15 EVT-named files are not
  listed maps; one (`d4bm.evt`) is not a framed script and must not be fed to
  the current event parser. The ignored `event-probe.json` retains every record.
- [ ] Resolve whether any unlisted EVT file is reachable through a hidden map
  or event path before treating it as ignorable.
- [ ] Decode and implement every listed-map/global EVT operand and side effect.
  Specifically, active opcodes 20, 27 and 28 are absent from the current C++
  enum. The [MM6 level editor's event format source](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java)
  identifies them as item modification, random password and random answer;
  confirm operand layouts against shipped records and game behavior.
- [ ] All 76 `.str` string resources and their map/event references.
- [ ] Remaining 11 `.bin` data families and 14 `.fnt` font files.
- [ ] Video and audio archives, soundtrack mapping, playback and fallbacks.
- [ ] Compare one low, middle and high record of every ID family and every
  structure boundary with MM7. Record incompatible assumptions explicitly.

## Engine and campaign behavior

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
  enchanting, artifacts, quest items, shops and item generation.
- [ ] Every spell ID 1–99 with Normal, Expert and Master behavior: target rules,
  mana, recovery, damage, resistance, status, buffs and world effects.
- [ ] Real-time and turn-based combat, projectiles, monster AI, pathfinding,
  loot, special attacks and resistances.
- [ ] Outdoor and indoor loading, transitions, weather, time, rest, respawn,
  traps, chests, doors, switches and map reset rules.
- [ ] EVT interpreter coverage for all shipped scripts; NPC dialogue, houses,
  guilds, quests, awards, travel, hirelings and endings.
- [ ] Audio, music, movie playback and settings on Apple Silicon.
- [ ] Separate `.ec6` save writer and reader: all party, inventory, quest,
  NPC, world, map delta, time and configuration state; atomic writes, versioning,
  backup and reload after exit.
- [ ] Private MM6 portrait and armor override hooks, each with Vanilla bypass.
- [ ] Offline validation for every table row, map and event resource, plus
  scenario checks for critical campaign branches and an ending path.
- [ ] Live visual and play validation from New Sorpigal through ending, with
  save/quit/relaunch/load at multiple checkpoints.

## Trilogy coexistence contract

- [x] Three source worktrees exist side by side: MM6, MM7 and reserved MM8.
- [ ] All three are independently built as their own title-specific engines.
- [ ] Three sibling bundles inside The Atlas: `MM6.app`, `MM7.app`, `MM8.app`.
- [ ] Three independent game-data roots, profiles, save formats, mod folders,
  preferences, caches and logs. A title may read only its own runtime data.
- [ ] Rebuilding or installing one title preserves the other two app bundles,
  their saved games and their Atlas launch actions.
- [ ] The Atlas marks each campaign playable only after its own verified
  graphical launch, campaign progression and save/load checks.
- [ ] Transfer directions unlock only when both corresponding engines and
  import/export adapters pass independent checks.
