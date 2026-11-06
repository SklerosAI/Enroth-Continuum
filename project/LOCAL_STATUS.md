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
  and exact 67-name MAPS staging validator. The offline suite passed 200 tests from 59 suites
  across EVT framing and selected guarded effects, wire codecs and indoor/outdoor projection,
  variable guard, `.ec6` fields and partial assembly, private transport/spell
  rules, descriptor 173 and LOD order; the
  private actor fixture checked 67 map deltas/342 actors and one ID 173. The
  offline runner also verified the F3-matched mode-0600 map-name manifest and
  private First Aid, Power Cure, timed-cure and Shield/Heroism rules. The potion
  table check covered all 841 retained cells, preserving opaque markers.
  Known-save map validation read all 67
  retained base/delta pairs through the F3-verified private map source before
  staging a candidate, while live SaveLoad stays blocked. The native atomic disk helper now
  rejects CRC-valid files whose sixteen known sections fail semantic decode.
  A read-only slot inventory lists valid or recovery-needed `.ec6` files,
  and a caller-validated load preflight selects a primary or backup without disk mutation. MM6 item-icon and
  high-row NPC lookup guards also passed focused checks. MM6 autonote ID 205 and
  class/item event-operand bounds have title-specific offline paths.
  MM6 condition events now execute 18 bounded retained uses while 36 status
  writes remain guarded. The monster parser distinguishes seven attack damage
  labels, with five unproved channels stopped before dice or state mutation.
  Six MM6 resistance columns are preserved explicitly; Energy remains unmapped.
  All 93 active monster special labels and 55 monster spell cells now retain
  their MM6 types/IDs, while their effects remain gated. All 82 active projectile cells retain MM6 types; nine missile and seven impact objects join 85 archive-matched frames, while Dagger remains unresolved and ranged launch stays guarded.
  A read-only First Aid spellbook preflight records transaction inputs; live
  casting remains guarded. The house parser classifies labels on all 557 retained house rows,
  including 384 active rows that previously chose the mercenary guild type;
  services and graphical house screens remain unverified. MM6 house entry now
  validates title-specific room/media data for a display-only preview while service actions remain guarded. Runtime MM6 pre-entry validates the active 557-row table and private
  119-room manifest, classifies the entry, and permits only table-backed display-only previews; blank and terminal routes stay unavailable. The read-only resolver classifies all 674 active
  house-entry instructions. Fifty-one active proprietor picture IDs bind to retained
  icons and preflight metadata before UI state changes. The staged F3-matched
  archive decodes all 45 distinct active proprietor images; display is unverified.
  A read-only 557-row service data decoder preserves
  MM6 stock text, route notes, values and hours. A pure selector joins all 21
  private transport houses to retained rooms and exposes nongated routes. Exact extensionless
  movie lookup covers all 127 F3-matched VID names offline. Failed house
  loads and loop restarts preserve playback state; one staged room frame now
  decodes offline, while on-screen playback remains unobserved. The award loader retains all 100 numbered rows;
  seven formatted texts stay guarded pending title-specific counters, and
  priority/order remains provisional. The award event guard bounds all
  193 active operands to the 100-row MM6 table. A separate audit covers all
  611 active quest-bit uses. A title-specific branch now reads and changes the
  bounded party bit once per instruction without inherited MM7 cues; journal
  behavior and full persistence remain open. A separate ID-19 branch compares
  the 336-day calendar for all 592 retained active uses. Gold ID 21 bounds
  147 active compares, grants and payments with silent party balance changes.
  Raw item ID17 has 641 active uses: 195 read-only comparisons and 446 audited writes.
  A bounded MM6 branch now handles Add/Subtract using the accepted original-PE dispatch.
  Of 290 Add sites, 288 have eligible static operands/selector, with cursor capacity
  checked at runtime; the artifact-band and party-selected sites remain guarded.
  All 156 Subtract operands are statically eligible; an unproved full-inventory ground drop stays guarded; duplicate backpack
  matches now follow first-grid-cell priority.
  Seven spell damage source tags are retained for all 99 MM6 IDs; live effects
  remain gated. MM6 Escape clears a pending targeted cast queue entry and target selection rejects invalid party-slot indices;
  live casting is still gated.
  The general non-MM6 unit suite passed 438/438 tests from the same build.
  F4 rules, map state, events, services, media output and saves remain open.
  The owner-local MM6 executable was inspected read-only once under its accepted F3 hash; no MM6 process was launched.
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
  assembly. RCVR v2 preserves 59 event/history clocks from PartyTimeStruct;
  a non-mutating HIRE/RESC diagnostic reports possible matches but
  cannot establish stable source identity. MM6 day/rest behavior and active
  turn/rest recovery remain unresolved. The MAPS exact-set and retained base/delta checks pass offline. A detached
  world-save adapter now captures actual NPC/group state and all 67 map buffers,
  invoking the bounded active-map writer before overlaying the current delta;
  the outdoor ID/table/HEAD/manifest join is checked, while live outdoor
  transition identity and the complete save transaction remain open.
- The private F3-derived transport manifest is read by a title-specific
  three-option MM6 menu. It hides unproved quest, weekday and special routes;
  selection makes no payment or departure while the `.ec6` save transaction
  is incomplete. MM7 transport tables are never indexed for MM6 houses.

## MM6 F4 integrated checkpoint — 2026-11-06

The arm64 MM6, OpenEnroth and unit targets linked. The one-command private-backed
offline gate passed **200/200 tests from 59 suites**, and the separate general
suite passed **438/438**. This checkpoint adds the display-only MM6 house window
with MM6 panel/exit art and title-specific entry/exit routing, bounded item
Add/Subtract writes including inventory-success occupied-cursor relocation, a
private-backed detached First Aid character transaction, CHAR v2 character
modifier capture, and detached Timer/50-overlay save staging. See the
[house presentation](MM6_F4_HOUSE_PRESENTATION.md),
[item PE transaction](MM6_F4_ITEM_EVENT_PE_TRANSACTION.md),
[First Aid effect](MM6_F4_FIRST_AID_EFFECT.md), and
[save transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md).

The owner-local MM6 executable was inspected read-only under its accepted F3
identity for item-event semantics; the original installation was otherwise
left untouched. No MM6 process or graphics/audio play was launched. Full MM6
spell effects, house services, artifact/ground-drop behavior, mandatory save
state and all-owner load commit remain F4 gates. The MM7 app/profile and
reserved MM8 checkout were not changed.

The later offline additions decode one bounded first frame from a private
F3-matched MM6 room movie through the existing Smacker converter and reject
malformed/truncated samples. Raw ID-17 Subtract now removes the first matching
backpack grid cell, independent of item-record insertion order. Outdoor save
capture checks current map ID, loaded table `.odm`, `HEAD` and the unique
private-manifest `.ddm` before encoding. These add two tests to the integrated
MM6/LOD gate. Actual house playback, outdoor transitions and save/reload remain
unobserved; the launch gate stays closed.

## MM6 F4 integrated checkpoint — 209 tests, 2026-11-06

The arm64 MM6, OpenEnroth and unit targets linked; the one-command private-backed
offline suite passed **209/209 MM6/LOD tests from 61 suites**, and the separate
general suite passed **438/438**. Every one of the 116 distinct MM6 house room
movies referenced by the retained table produced a bounded first decoded frame
without launching a game. One F3-matched active event sound resolved through
the MM6 sound table and staged SND archive to nonempty PCM without playback.
The detached Power Cure transaction stages mana, capped healing and recovery
for four copied characters; its live cast guard remains closed.

The 67-map save validator checks populated MM6 templates for positive ground
and chest item IDs, accepts only the six observed negative chest random-item
markers, and rejects foreign IDs before assigning a candidate. The inherited
MM7 chest generator could select IDs that are MM6 message scrolls, so a new
title guard blocks **every MM6 level load** before chest, RNG or artifact
mutation. The MM6 generator, chest grid, shop save identity, full `.ec6`
transaction, spell effects and services remain F4 work. No MM6 process was
launched; the installed MM7 app/profile and reserved MM8 checkout were not
changed. Proprietary assets and detailed reports remain ignored locally.

## MM6 F4 integrated checkpoint — 214 tests, 2026-11-06

The arm64 MM6, OpenEnroth and unit targets linked after the next isolated
workstreams were integrated. The private-backed offline gate passed **214/214
MM6/LOD tests from 61 suites**; the separate general gate passed **438/438**.
No MM6 process, graphical view or audible playback was launched.

A source-identity-checked MM6 executable trace established a one-HP maximum
health floor and a distinct 7 × 7 chest marker remap. The chest trace also
identified original outer RNG behavior and title-specific artifact/item paths;
the detached remap tests pass, but the live chest guard still stops every MM6
level load. The original Quick Start choice and four-character class, face,
name-reference, stat and skill fixture are now documented, with generated
starter inventory, voice and final spell state still unresolved. A staged
house greeting candidate is read-only and plays no sound.

Detached `.ec6` `CHAR` v3 now preserves quick spell and spellbook page. A
partial save coordinator validates all known sections and all 67 map pairs
before staging NPC/map and timer/overlay owners on copies. Live SaveLoad remains
closed: active actors/chests, PID rebinding, other state owners and the final
commit are unproved. Exact PE evidence and GOG assets stay in ignored local
storage; the installed MM7 app/profile and reserved MM8 checkout were not
changed.

## MM6 F4 integrated checkpoint — 224 tests, 2026-11-06

The arm64 MM6, OpenEnroth and unit targets linked after the next isolated
changes were integrated. The private-backed offline gate passed **224/224
MM6/LOD tests from 62 suites**; the general non-MM6 gate passed **438/438**.
No MM6 process, graphical view or audible playback was launched.

The original-executable chest trace now has detached, seeded MM6 marker remap,
base-item ID and ANY item-field models. They stage artifact flags and RNG plus
identification, enchantment and wand-charge fields without mutating the live
game. The title guard still blocks every MM6 level load because retained-table
callers, other item sources, chest placement, grid encoding and persistence
are unresolved. Quick Start's sixteen starter-skill paths, one face-derived
sound selector and bounded first-map script were audited; random inventory,
remaining audio gaps and full loader effects still block its live route.

The partial `.ec6` stage now rejects saved overlay actor PIDs outside the
validated active-map actor count. Original MM6 shop/guild handlers prove
row-keyed refresh and raw Party-block persistence, but stock slot meaning and
the title-specific save tuple are not yet typed. Live SaveLoad and house
services remain guarded. All detailed PE evidence and proprietary data remain
ignored locally; the installed MM7 app/profile and reserved MM8 checkout were
not changed.

## MM6 F4 integrated checkpoint — 233 tests, 2026-11-06

The arm64 MM6, OpenEnroth and unit targets linked. The private-backed offline
gate passed **233/233 MM6/LOD tests from 63 suites**, including 192 seeded ANY
item stages against an ignored F3-matched chest profile; the general gate
passed **438/438**. No MM6 process, graphical view or audible playback was
launched.

The original chest trace now has detached retained-table caller, gold-source,
ANY finalization and 140-cell first-placement models. They do not mutate live
state. Direct tier-seven artifact selection, changed chest-grid persistence,
entry-side integration and the all-owner `.ec6` transaction remain open; the
live title guard still stops every MM6 map load. A separate original outdoor
border trace bounds the neighbor grid, travel-day and arrival shape, while
party eligibility is untyped and MM6 border travel stays guarded. MM7-only
event exceptions, border/fog tables and underwater-map classification are
title-gated. Cross-map transitions still hit the MM6 autosave guard before
scheduling travel. The installed MM7 app/profile and reserved MM8 checkout
remain untouched; exact game/executable evidence stays ignored locally.

## MM6 planning update — headless integration, 2026-11-06

After the 233-test checkpoint, detached chest outer-loop, direct-artifact,
changed-grid wire and type/layout code was integrated. Those commits have not
yet had a full combined build or offline test run; the live MM6 chest and
SaveLoad guards remain closed. The [revised plan](plan.md) makes an MM6-specific
headless game-test harness the next integration prerequisite. Current source
uses a null platform, renderer and audio driver for headless startup, but the
existing game-test runner still defaults to MM7 and has no MM6 selector.
No MM6 headless process, foreground process, graphical view or audio playback
was run for this planning update. The goal remains paused.

## MM6 F4 refreshed integrated checkpoint — 249 tests, 2026-11-06

The main arm64 `MM6`, `OpenEnroth` and `OpenEnroth_UnitTest` targets linked
after the detached chest outer-loop, direct-artifact, grid-wire and type/layout
commits. The private-backed offline runner passed **249/249 MM6/LOD tests from
66 suites**; the general non-MM6 runner passed **438/438 tests from 56 suites**.
This supersedes the prior 233-test checkpoint as the current build baseline.
The chest generation and MM6 SaveLoad guards remain closed. No MM6 process,
rendered view or audio playback was launched; the installed MM7 app/profile
and reserved MM8 checkout remain untouched.

## MM6 F4 headless and joined staging checkpoint — 262 tests, 2026-11-06

The main arm64 unit and game-test targets linked after the detached chest
record generator, first-open placement, runtime projection, changed-grid
writers and partial `.ec6` Party/Character/world candidate. The private-backed
offline gate passed **262/262 MM6/LOD tests from 68 suites**; the general
non-MM6 gate passed **438/438 from 56 suites**. The MM7 headless game-test
control passed **1/1**.

The MM6 game-test runner now selects the title explicitly, reads eight private
rule manifests into an in-memory profile before engine initialization and
uses null platform, renderer and audio. A monitored MM6 run passed **4/4
headless tests**; across the two-test engine smoke, 102 host samples found no
focus change or on-screen window. These tests verify startup and the existing
chest and new-game save guards. They do not load the first map or validate
graphics or audible playback. Normal new-game flow reaches the `.ec6`
`saveNewGame()` guard before map entry; a separate chest-generation guard also
blocks map load. Current menu requests some MM7 icon names absent from MM6
data. Both guards remain closed while the atomic chest handoff, complete
save transaction and MM6 menu assets are built. Private game data, rule files
and logs remain ignored locally; installed MM7 and reserved MM8 are untouched.

## MM6 F4 menu, CHAR v4 and chest handoff checkpoint — 269 tests, 2026-11-06

A fresh arm64 build of `MM6`, `OpenEnroth`, unit tests and game tests passed
after the title-specific MM6 main menu, `CHAR` v4 bonus-byte preservation,
fail-closed commerce/beacon checks and detached atomic 20-chest handoff. The
private-backed offline gate passed **269/269 MM6/LOD tests from 69 suites**;
the general unit gate passed **438/438**. The explicit MM6 headless suite passed
**6/6** and the MM7 headless control passed **1/1**. A monitored MM6 run saw
no focus change or on-screen window across 31 host samples. The MM6 title
button resources now resolve in headless startup; visual alignment and click
behavior still need a graphical check.

The chest handoff is detached from live `vChests`, original MM6 LCG and
artifact owners and `.ec6` save state. New-game saving and map loading remain
guarded. The partial save candidate rejects active beacon and inherited
MM7-indexed commerce state rather than omitting it. First-map play, HUD art,
save/reload and campaign effects remain open.

## MM6 F4 first-map and HUD structural checkpoint — 277 tests, 2026-11-06

The offline first-map autosave candidate validates all 67 default deltas
before an atomic `.ec6` write. A title-local chest owner and opt-in adapter
can stage actual `vChests` and active indoor/outdoor sidecar bytes without
mutating a failed candidate. The native private-backed suite passed
**277/277 MM6/LOD tests from 71 suites** after those additions. Both live
SaveLoad and chest-generation guards remain closed; initial LCG seed,
party-owned artifact state, complete `.ec6` reload and first-open behavior
are not yet joined.

The HUD now has MM6-specific compass/zoom candidates and four source-backed
frame assets at title-specific positions. The linked headless MM6 suite
passed **10/10**, and the MM7 control **1/1**. A monitored MM6 run had no
focus change or on-screen window across 177 host samples. Remaining missing
requests include book and action icons; the four-book route and its visible
layout still need implementation and later graphical validation.
