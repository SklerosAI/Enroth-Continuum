# Local status — 2026-11-05

This is a personal-machine validation note. It contains no game assets.

## MM7: playable on this M2 Mac

- macOS 27.0.1, arm64. The fork's native OpenEnroth engine and The Atlas launcher
  build as arm64 and are installed together at `/Applications/Enroth Continuum.app`.
- The installed app opens the three-campaign Atlas screen. The MM7 button
  launches the bundled engine from the installed app, shows Emerald Island,
  and returns to the launcher on exit.
- A manual MM7 save was created, then loaded after exit and relaunch. Saves are
  under `~/.openenroth/saves`; a separate local backup was made through The Vault.
- A synthetic PNG placed through The Atelier rendered in party slot 1 after
  loading that save. A Leather Armor template AC override was accepted by the
  engine and logged as item 66, base 20, bonus 3. The test portrait and armor
  file were removed; the installed app now shows Vanilla mode with no override.
- OpenAL found local audio devices. Playback was not checked by ear.
- A full-campaign MM7 engine audit is planned in
  [MM7_AUDIT_CHECKLIST.md](MM7_AUDIT_CHECKLIST.md). The current graphical
  opening and save/reload check does not establish later map, rule, quest,
  alignment or ending coverage; the known-good MM7 build remains protected.

## MM6: implementation checkpoint, not yet playable

- Owned offline GOG installers were extracted into private, local folders under
  `~/Library/Application Support/Enroth Continuum/Game Data/`.
- Branch `mm6-mac-implementation` adds `--game mm6`, a separate MM6 user
  profile, MM6 table and event adapters, an MM6 archive decoder, and an MM6
  outdoor map decoder. The native arm64 build succeeds.
- The MM6 title screen renders from the owner's data. Party creation opens,
  but still uses MM7 layout and expects missing MM7 portraits and class icons.
  Pressing Return passes the default party through to the opening transition.
- The initial MM6 map attempt exposed the distinct compressed map wrapper. Its
  decoder now builds, but the map start change and decoder have not yet been
  checked together. The earlier attempt incorrectly entered MM7's first map
  and failed on its MM6 delta format. MM6 delta, indoor map, campaign behavior,
  and save/load support remain unfinished.
- The installed Atlas still has no MM6 Play button. The installed MM7 app was
  not replaced; its known-good save and launch path remain the baseline.
- The Atlas also detects MM8 core archives but has no MM8 Play button.
- MM8 game data includes `Music/2.mp3` through `Music/15.mp3`. A separate
  `mm_8_ost_flac.zip` download is in `~/Downloads` and contains FLAC soundtrack
  tracks; it has not been added to the game-data folder or Git.
- The MM8 offline installer did not list or extract a `Sounds/Audio.snd` archive.
  The soundtrack download establishes music availability, but the MM8 sound
  effects format/source still needs investigation when its engine adapter begins.
- Character transfer remains a previewed design in the Hall of Echoes. No save
  converter is enabled until a destination game can safely load its output.

The game archives, installers, save files, backups, and private mod files are
outside Git. `local-game-data/*` is ignored except its tracked README.

## Trilogy isolation checkpoint — 2026-11-05

- MM7 source is independently preserved on branch `mm7-isolated-runtime` at
  commit `020cecf49`, in a separate managed worktree. Its staged Atlas bundle
  contains `Contents/Helpers/MM7.app` with a distinct bundle identifier.
- The new MM7 profile is under
  `~/Library/Application Support/Enroth Continuum/Profiles/MM7`; the two
  existing save files were copied there and SHA-256 matched both the originals
  and a separate local backup. The old `~/.openenroth` profile remains intact.
- The isolated MM7 app built and passed a live graphical check: The Atlas
  launched MM7, an existing save loaded on Emerald Island, and quitting
  returned to The Atlas. The installed `/Applications` app was left in place
  during this verification.
- The private MM6 reference folder
  `local-game-data/mm6/research-2026-11-05` holds extracted copies of four
  LOD archives and an ignored per-file/per-table catalog. Archive entry lists
  contain 2,703 icons, 134 maps, 1,958 bitmaps and 4,314 sprites. The catalog
  indexes 9,128 extracted and manifest files and every nonempty row in 30 text
  tables. See [MM6_IMPLEMENTATION_CHECKLIST.md](MM6_IMPLEMENTATION_CHECKLIST.md)
  for implementation coverage and remaining audit work.
- MM6 is still not playable. The isolated MM7 bundle is a protected baseline;
  MM6 and later MM8 will be separate sibling engines and profiles, with their
  own Atlas actions once their campaign checks pass.
- Three source checkouts now exist simultaneously: `mm6-mac-implementation`
  (active adapter), `mm7-isolated-runtime` (verified baseline), and
  `mm8-isolated-runtime` (reserved untouched baseline). MM8 has no adapted
  engine or bundled app yet.
- The intended installed layout is three sibling helpers (`MM6.app`,
  `MM7.app`, `MM8.app`) and three private profiles. The current Atlas build
  script still packages a single engine as `OpenEnroth.app`; a title-aware
  assembler is required before adding MM6 or MM8 so builds cannot replace
  another game's helper.
- Offline MM6 probes cover all 134 retained map/delta entries, all 52 indoor
  `.blv` and `.dlv` layouts, all 15 outdoor `.odm` and `.ddm` layouts, and the
  67 listed map scripts plus `global.evt`. Every base and delta layout reaches
  its exact decoded end. Two `.blv` entries contain complete DEFLATE streams
  followed by extra bytes inside the archive entry;
  their runtime decoding policy remains open. Three opcodes used in listed map
  scripts still lack MM6-specific C++ parsing and execution; their payload
  lengths are now inventoried. All 15 unlisted EVT files lack matching archived
  base maps. Offline checks also match 67 map string/EVT pairs, all 11 binary
  table boundaries and all 14 font glyph atlases. Seven non-house mouseover
  hints remain out of range; opcode 5 and MM6 input-dialog behavior need
  resolution. These findings are recorded in the checklist; no installed
  runtime changed.
- The retained media audit confirms that all 12 map music IDs have MP3 files.
  None of MM6's 116 used house animation IDs maps to a video name in the
  inherited MM7 room table. All 227 active opcode-11/12 event resource names
  resolve to MM6 bitmaps; opcode 12 needs a title-specific two-field texture
  operation instead of the inherited MM7 movie interpretation. The ignored
  `media-reference-probe.json` records the reproducible checks.
- A separate offline parser compatibility audit covers every frame in the 68
  active MM6 EVT files. It finds at least 318 deterministic rejections in the
  current C++ parser, concentrated in monster spawning and six other opcodes.
  The private `event-parser-probe.json` keeps per-record offsets and source
  parser hash for the MM6 implementation pass.
- An MM6 variable-ID audit covers 4,285 active condition/reward operands.
  1,609 use IDs in MM6-specific ranges whose meanings diverge from the
  inherited MM7 enum, including 1,240 references to MM6 map bytes. The
  current engine has no MM6 variable dispatch or 100-byte map-variable layout;
  the ignored `event-variable-probe.json` maps each active use to its source
  event and inherited interpretation.
- A spell/item ID audit covers all 99 MM6 spells and all 581 item-table rows.
  Fifty spell names match inherited IDs under a strict lexical check; 21 have
  the same name at another inherited ID and 28 are absent or renamed. Mana
  differs at 27 IDs and 11 elemental spell tags currently default to physical.
  MM6 spell scrolls are IDs 200–298, spellbooks 300–398, and message scrolls
  500–549, whereas inherited MM7 helpers assign different categories to those
  ranges. Private ignored reports retain per-ID evidence for implementation.
- A locally retained, ignored copy of the 412-page Prima strategy guide is
  text-searchable. Its chapter outline has been converted into explicit
  campaign-coverage work in the checklist. This is source indexing, not yet a
  page-by-page reconciliation or proof of any implemented behavior.
- Focused F2 documents now cover [guide and campaign milestones](MM6_GUIDE_RECONCILIATION.md),
  [MM6 save and map-delta requirements](MM6_SAVE_SPEC.md), and
  [class, skill and party creation IDs](MM6_CLASS_SKILL_SPEC.md), plus
  [presentation and media routing](MM6_MEDIA_UI_SPEC.md) and
  [event/dialogue semantics](MM6_EVENT_CAMPAIGN_SPEC.md). The six MM6
  three-tier class families and 31-skill table do not fit the inherited MM7
  class/skill IDs or nine-class creation UI. The 67 retained map deltas have
  distinct MM6 layouts, and native MM6 save compatibility has not been
  established. The media review finds inherited house/movie and spell-sound
  routing that still needs MM6 definitions; video archive names and actual
  playback remain for F3 and F5. These documents specify work still required
  before a playable MM6 runtime.
- A deeper F2 source review confirmed the inherited MM7 default party,
  creation skill and potion-mixing paths are unsuitable for MM6. The retained
  MM6 delta baselines confirm fixed byte spans but contain no nonzero evidence
  for map-bit and tail semantics. Event analysis found MM6 Question records
  that the inherited parser rejects and labeled NPC topic references with no
  current event or special handler. Exact behavior and campaign dependency
  mapping remain open; no runtime was changed by these reviews.
- A static [text-table consumer audit](MM6_TABLE_CONSUMER_SPEC.md) matches
  24 of the 30 retained MM6 table names to direct C++ resource loads. Six
  have no literal loader reference and need indirect-use or non-use review;
  the audit does not establish that any loaded table is semantically correct.
  A reproducible offline probe records source locations in an ignored report.
- A focused review of the six tables without direct loaders found a complete,
  symmetric MM6 item-combination matrix in `useitems.txt`; the adapter does
  not load it, and result-token meanings remain unverified. The MM6 NPC
  profession loader also leaves a dismissal-text field blank although the
  dialogue UI reads it. Autonote, NPC template, password and incidental-file
  roles remain for F2/F3 reconciliation.
- A focused [monster/combat source audit](MM6_MONSTER_COMBAT_SPEC.md) found
  active MM6 attack, projectile, spell and special-attack labels that fall
  through inherited MM7 parsers. The adapter also retains MM7 monster-family
  rules and a temporary all-friendly monster faction table. These are
  source/retained-table findings; MM6 combat has not been played or verified.
- A private structural campaign graph now links retained event functions,
  map moves, house exits, quest-bit reads/writes and NPC topic changes.
  Explicit script and house map destinations all resolve to listed MM6 maps,
  but guide milestone matches are candidate links only. Control flow,
  quest meaning and ending reachability remain unverified; details stay in
  ignored `quest-graph-probe.json`.

## MM6 F3 read-only preflight update — 2026-11-06

- F2 is [frozen for F3](MM6_F2_GATE_REVIEW.md). The 30 text-table families,
  six formerly unclassified table roles, nine extra STR resources and all
  61 manual, five card and eleven NWC guide pages have F2 source/retained-data
  coverage. This supersedes the earlier open-role and page-review status above.
- The original extracted installation passed **23/23** file identities:
  eight core archives/media/executable and 15 soundtrack tracks. A bounded
  original directory snapshot found no content-name mismatch. A targeted
  [content reconciliation](MM6_F3_CONTENT_RECONCILIATION.md) compared 348
  original table, binary, font, map and event members byte for byte with the
  private retained extracts. Two map checksum anomalies and one extra event
  framing anomaly are confirmed in original bytes and have scoped follow-up.
- The [PE reconciliation](MM6_F3_PE_RECONCILIATION.md) mapped creation,
  spell-delay, house-movie and transport tables. Exact values remain private;
  one spell-cost and one weekday-source disagreement are documented. Travel
  map indices require the original Games archive directory order, which the
  retained name list does not preserve.
- The [media reconciliation](MM6_F3_MEDIA_RECONCILIATION.md) checked original
  video/sound payload framing and all soundtrack streams, and statically
  resolved the referenced house movie names. It has not rendered video frames
  or portraits or confirmed audible playback.
- The [F3 gate review](MM6_F3_GATE_REVIEW.md) accepts the original-data
  reconciliation for offline F4 work. Executable follow-up confirms static
  consumers for spell cost, class mana attribute and travel weekdays;
  portrait/voice reconciliation verifies 636 vanilla portrait pixels; one
  baseline map actor references monster ID 173 and needs the final descriptor.
  These are static findings, not runtime or visual/audio acceptance. No MM6
  runtime was launched during F3; the installed MM7 app, saves and MM8 reserved
  checkout remain untouched. MM6 still has no Play action or working campaign.

## MM6 F4 offline implementation update — 2026-11-06

- A dedicated native arm64 `MM6.app` **build target** now has its own bundle
  identifier, defaults to MM6, rejects `--game mm7` and MM7 trace playback,
  and resolves the MM6 Mac data/profile roots. The separate OpenEnroth target
  still builds with its existing bundle identity. This staged build was not
  launched or installed and is not an Atlas Play action.
- The Games LOD reader preserves original directory order; MM6 transport
  destination lookup no longer relies on its sorted `ls()` result. The loader
  now retains all 173 monster descriptors, including the one referenced by a
  baseline actor. The [portrait seam](../.llm-wiki/MM6-F4-Portrait-Seam.md)
  uses the twelve verified face families and 53 frames per face, with
  provisional initial faces/voices pending creation-rule work.
- The [active EVT framing seam](MM6_F4_EVENT_FRAMING.md) parsed all 68 listed/
  global retained resources through the C++ reader. It explicitly blocks
  unresolved MM6-only effects. The [`.ec6` envelope](MM6_EC6_CONTAINER_F4.md)
  has title/version, bounds and integrity checks. A [core field codec](MM6_EC6_CORE_FIELDS_F4.md)
  now stages party/character data from a checked container into a copy; the
  durable quest/award/condition/inventory fields also have bounded codecs;
  nonempty indexed inventory restoration, a native atomic writer, and a partial
  sixteen-section assembly are present offline. SaveLoad and load-menu integration
  remain guarded, and complete state coverage is open.
- The [outdoor wire codec](MM6_OUTDOOR_WIRE_F4.md) checked all 15 retained base/
  delta pairs; the [indoor wire decoder](../src/Engine/Snapshots/MM6IndoorSnapshot.h)
  checked all 52 original-matched retained pairs. Both preserve opaque bytes
  and reject malformed counts. The MM6 outdoor loader now projects reset,
  visibility, actor, ground-item and chest records into runtime structures.
  Bounded indoor and outdoor changed-state encoders preserve the raw sidecar,
  while unsupported live changes fail closed. The MM6 indoor loader projects
  52 checked BLV/DLV pairs into runtime; all 67 deltas round-trip in `.ec6` offline.
  The [creation rules foundation](MM6_F4_CREATION_RULES_FOUNDATION.md) reads a
  private profile manifest and adds six starting class choices; formulas,
  Quick Start and promotion remain provisional. The [variable guard](MM6_F4_VARIABLE_GUARD.md)
  prevents MM6-specific event IDs from executing as colliding MM7 meanings.
- The integrated MM6 app, MM7 app and unit targets built with the sixteen-section
  partial assembly, spell-item prerequisite guard, MM6-only effect-dispatch guard,
  and exact 67-name MAPS staging validator. The offline suite passed 136 tests
  across EVT framing and selected guarded effects, wire codecs and indoor/outdoor projection,
  variable guard, `.ec6` fields and partial assembly, private transport/spell
  rules, descriptor 173 and LOD order; the
  private actor fixture checked 67 map deltas/342 actors and one ID 173. The
  offline runner also verified the F3-matched mode-0600 map-name manifest and
  private First Aid, Power Cure, timed-cure and Shield/Heroism rules. The potion
  table check covered all 841 retained cells, preserving opaque markers.
  Known-save map validation read all 67
  retained base/delta pairs through the F3-verified private map source before
  staging a candidate, while live SaveLoad stays blocked. MM6 item-icon and
  high-row NPC lookup guards also passed focused checks. MM6 autonote ID 205 and
  class/item event-operand bounds have title-specific offline paths.
  MM6 condition events now execute 18 bounded retained uses while 36 status
  writes remain guarded. The monster parser distinguishes seven attack damage
  labels, with five unproved channels stopped before dice or state mutation.
  A read-only First Aid spellbook preflight records transaction inputs; live
  casting remains guarded. The house parser classifies all 557 retained labels,
  including 384 active rows that previously chose the mercenary guild type;
  services and graphical house screens remain unverified.
  The general non-MM6 unit suite passed 438/438 tests from the same build.
  F4 rules, map state, events, services, media output and saves remain open.
  No MM6 process or original GOG file was opened for these F4 checks.
  A title-specific zero-mastery `CheckSkill` branch now has a provisional
  source-backed rule and focused threshold test; its quest outcome remains F5.
  The retained outdoor target check supports zero-based texture targets (12/12)
  and all-faces facet targets (6/6); those runtime effects are not yet saved.
  MM6 Question now has a provisional text-entry and two-answer continuation
  path, with visible and original-behavior checks still pending.
- A private F3-backed spell manifest covers all 99 IDs at three mastery ranks.
  Regular player-cast cost/recovery uses it; all 99 MM6 regular effect IDs
  currently reject before inherited player/event/monster/damage dispatch, and
  scroll/book use remains guarded. A retained-data verifier checked all 99 scroll and 99 book
  IDs against MM6 spell IDs. The raw `RSTS` resistance section captured all 88 current
  character values. Raw timed buffs and the cursor-held item are also bounded
  and required by the revised partial assembly. Their MM6 effects, buff-overlay
  references and post-load cursor presentation remain open. Shop/map items,
  generated artifacts and complete save/relaunch remain open. Raw hireling copies
  and recovery/calendar fields are now bounded and required by the partial
  assembly; a non-mutating HIRE/RESC diagnostic reports possible matches but
  cannot establish stable source identity. MM6 day/rest behavior and active
  turn/rest recovery remain unresolved. The MAPS exact-set and retained
  base/delta checks pass offline, but remain unwired from live capture and the
  complete save transaction.
- The private F3-derived transport manifest is read by a title-specific
  three-option MM6 menu. It hides unproved quest, weekday and special routes;
  selection makes no payment or departure while the `.ec6` save transaction
  is incomplete. MM7 transport tables are never indexed for MM6 houses.
