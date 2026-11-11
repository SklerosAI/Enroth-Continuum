# Local status — 2026-11-05

This is a personal-machine validation note. It contains no game assets.

## Atlas headless reinstall — 2026-11-10

After the owner authorized reinstalling Atlas, the signed MM7 recording
candidate was installed at `/Applications/Enroth Continuum.app` without
launching it. The previous app was moved to an Application Support rollback
directory. The installed page and Bash recorder hashes match tracked source,
the bundle and title-locked MM7 helper pass deep signature verification, and
no normal save or profile was changed. The owner currently permits headless
checks only, so post-install launch, media, Save/Load and real recording remain
unverified. The November 7 install below is historical.

## MM7 Atlas install — 2026-11-07

- `/Applications/Enroth Continuum.app` now carries a single Release `MM7.app`
  helper built from the integration tree's title-locked `MM7` target. It reads
  `Game Data/MM7` and the `Profiles/MM7-Atlas` profile, which was seeded with
  the previous profile's settings and every earlier MM7 save.
- A visible main-display run of the Release helper replayed a recorded trace
  that loads a save and plays input. It exited cleanly after 15 seconds. The
  installed Atlas opened and stayed up. Automated screenshots of the visible
  run were not possible without macOS Screen Recording permission.
- The superseded Debug `OpenEnroth.app` helper, the upstream nightly DMG, the
  `~/.openenroth` profile, the duplicate `Application Support/OpenEnroth` data
  copy and the interim `Profiles/MM7` profile were deleted at the owner's
  request. `OPENENROTH_MM7_PATH` in the private toolchain now points at
  `Game Data/MM7`.

## MM6 update — 2026-11-06

The historical MM6 snapshot below predates the current F4 work. The separate arm64 MM6 branch captures a test-only hidden OpenGL first-map frame with four portraits, health/mana bars, foliage, sky, buildings and road art. A fresh owner-only capture passed its scoped headless check without an on-screen window or focus change. Current pale terrain is likely a month-zero seasonal grass-to-snow mapping; original MM6 season behavior is still unproved. The editable New Game path reached its first outdoor frame in a dedicated hidden GameTest using a test-only temporary-profile `.ec6` save override and title-specific starter items. Its 67-map source now comes from the loaded MM6 archive and map table; a monitored private comparison matched all bases and names to the accepted F3 source. A second fresh headless process read the disposable `.ec6`, staged its represented owners and built a detached first-map plan without changing disk bytes. A detached native v2 first-map writer validates explicit physical inventory, cursor and fresh shop owners and atomically writes only an admitted candidate; synthetic linked checks passed 3/3. A bounded creation-item adapter produces raw records for supported fresh backpack grants in a detached Party copy. One deterministic private-backed headless run admitted all 20 starter items (16 plain, four standard-bonus) and wrote no save slot; other random draws and original-game byte parity remain unverified before the writer can be wired into New Game. The normal MM6 SaveLoad guard is still closed. The MM7 vendor mod control passed two real headless shop/guild entries after this change. MM6 still has no accepted playable campaign, normal save/load, live save–quit–relaunch–load cycle or Atlas Play button. The installed and frozen MM7 baseline remains separate.

A later disposable two-process test wrote a native v2 first-map `autosave.ec6` from the sampled real creation Party, then selected and staged it read-only in a fresh null-renderer MM6 process. Both linked tests passed 1/1 in the integration build; the owner-only system-temp profile was removed. A separate bounded nine-seed headless sweep admitted all 180 sampled starter items (155 plain, 25 standard-bonus) without a save slot; this does not cover every random draw or original-game record parity. None of these checks publishes all v2 owners into `gameLoop()` or opens production MM6 loading.

A third fresh null-renderer process exercised the v2 test-only load callback against that exact slot. The read-only admission gate named missing world/resource owners and refused publication; sampled live owners and disk bytes stayed unchanged. All three processes passed 1/1 in the integration build. The Main Menu QuickLoad event remains gated because the current void callback would otherwise continue into `gameLoop()` before complete final-owner validation can arm the post-world hook.

The one-shot MM6-only test hook now runs after `DoPrepareWorld(true)` and before timer unpause or first input. Four unit checks passed, and a disposable v1 Main Menu reload control passed 1/1 through the real hook with its slot unchanged; MM7 headless control passed 1/1. The v2 callback does not yet arm this hook with complete final-owner checks, so its Main Menu QuickLoad action remains closed.

A read-only NPC resource identity probe now matches the loaded MM6 static tables and global dialogue topics in a fresh null-renderer process (1/1). The first run hit a test-thread stack edge; heap-owning the large baseline and extra snapshots resolved it, and focused NPC/raw-session tests passed 18/18. Generated NPC identity and game RNG are still outside the saved owner contract.

## MM7: playable on this M2 Mac

The current integration tree also builds a title-locked `MM7.app` and stages a signed Atlas with `Contents/Helpers/MM7.app`. A wrong-title helper was rejected without changing the previous staged app, and a reuse build retained an identical MM7 helper. This is a build-tree checkpoint; the installed Atlas still has the prior generic helper until a live launch and save check accepts the new bundle. The protected `mm7-isolated-runtime` baseline and both local save profiles remain untouched.

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

The [MM7 source-autonomy inventory](MM7_SELF_CONTAINED_BUILD_INVENTORY.md)
records the original 23 gitlinks, nine prebuilt static libraries and seven
nested fetches. Nineteen full pinned snapshots, three scoped production-source
copies and five pinned Mac media-library source trees are tracked with original
licenses and provenance; ztd.text remains the single gitlink for non-local-only
builds. The original [media-library recipe](MM7_LOCAL_DEPENDENCY_RECIPE.md)
built nine arm64 static archives with copied notices.

The [clean offline build packet](MM7_CLEAN_OFFLINE_BUILD.md) now records a fresh
tracked-source extraction in which a network-denied sandbox built the nine
media archives, configured and linked the arm64 MM7 target, and staged a signed
Atlas with 23 dependency notices and no private game data. The opt-in
`EC_LOCAL_ONLY_CONFIGURE=ON` path validated all local archive hashes/source
pins, used tracked libdwarf and a scoped Mac iconv adapter, and avoided the
OpenEnroth dependency ZIP and ztd fetch path. A freshly built headless MM7
GameTest control passed 1/1 against the private default data root. The
ordinary CMake path still references the upstream download, and this was a
source archive rather than a remote clone. Visible play, save/quit/reload,
installed-app cutover and binary distribution review remain open. The existing
MM7 installation and distinct legacy save roots remain untouched.

## Installed Atlas MM6 playtest candidate — 2026-11-11

The signed `/Applications/Enroth Continuum.app` now contains separate,
title-locked `MM6.app` and `MM7.app` helpers and exposes gated actions for both.
MM6 requires its own data folder and writes only to `Profiles/MM6-Atlas`; it
does not inherit MM7 mods or the MM7 recording collector. The prior installed
Atlas was preserved as a local rollback copy before replacement. The staged
Atlas visual check used the Gridiron Dark interface. No MM6 owner session was
started as part of this installation, so this is a playtest candidate rather
than a visible-campaign acceptance claim.

## Historical MM6 implementation checkpoint, before the playtest candidate

The [indoor v6 input-cycle packet](MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md)
records five fresh headless children passing 1/1 each: seed, first-map
baseline, genuine indoor live-session writer, bounded real-loop turn/walk and
post-input atomic writer, then exact fresh-child replay. The test fixture
uses validated native floor geometry for placement and captures only
source-backed, monotonic DLV outline visibility changes. This remains a
test-only, one-indoor-map gate: production MM6 SaveLoad, general indoor play,
graphics and Atlas MM6 Play are still closed.

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
  `~/Library/Application Support/Enroth Continuum/Profiles/MM7`; an earlier
  private save copy was byte-verified against its retained source and backup.
  The old `~/.openenroth` profile remains intact.
- The isolated MM7 app built and passed a live graphical check: The Atlas
  launched MM7, an existing save loaded on Emerald Island, and quitting
  returned to The Atlas. The installed `/Applications` app was left in place
  during this verification.
- A private MM6 reference catalog was generated from the owner's local game
  files and retained outside Git. See
  [MM6_IMPLEMENTATION_CHECKLIST.md](MM6_IMPLEMENTATION_CHECKLIST.md) for
  implementation coverage and remaining audit work.
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

## MM6 F4 raw-terrain and hidden-render checkpoint — 2026-11-06

The integrated MM6 headless suite passed **47/47** over 1,526 monitor samples,
with no visible window or focus change. The private-backed offline gate passed
**327/327** MM6/LOD tests from 84 suites after the original 28-group terrain
sound route was integrated. MM6 now retains raw tile-table tileset and variant
IDs alongside its normalized runtime type; outdoor footsteps use that original
identity, while MM7 retains its previous lookup. Focused sound and all-map
selector tests passed **2/2**, the general non-MM6 unit gate passed **437/437**,
and an MM7 headless control passed **1/1**.

A test-only hidden first-map OpenGL recapture shows land and road art after the
MM6 land texture was assigned its shader's GPU texture unit. The sampled ground
region has zero exact-black pixels, and the capture did not take focus or open
an on-screen window. The original 17-condition priority correction also passed
linked unit and headless checks. Seasonal/shoreline fidelity, audible playback,
complete save transaction, normal MM6 map entry and campaign play remain open;
normal chest generation and SaveLoad guards remain closed.

The MM6-only `.ec6` `PART` v3 payload now preserves the current runtime's fine,
death, prison and bounty counters alongside the earlier party fields. The
integrated focused core-field suite passed **7/7**; the private-backed offline
gate passed **327/327** and a monitored scoped first-map save/rollback check
passed **1/1** over 1,571 samples without a visible window or focus change.
This is a detached preservation seam: original MM6 justice/award semantics,
full save commit/reload and normal SaveLoad remain unaccepted. A read-only
terrain schema audit also found that MM6 global tile ID is the exact art key;
raw tileset/variant pairs alone repeat, and the original seasonal remap rule
is still unproved.

## MM6 F4 stock, event and save-schema checkpoint — 2026-11-06

The integrated MM6 headless suite passed **49/49** over 3,414 monitor samples
with no visible window or focus change; the private-backed MM6/LOD unit gate
passed **331/331** from 85 suites. The general non-MM6 unit gate passed
**437/437**, and an MM7 headless control passed **1/1** without window or focus
change. MM6 `RCVR` v3 now preserves the detached
current-runtime Armageddon timer/damage/force tuple, and partial session staging
clears frame-derived rounding and delayed speech state while retaining saved
recovery. Live spell effects and the full save transaction remain guarded.

F3-matched original-executable traces establish that successful MM6 shop/guild
purchases transfer all 28 stock bytes and clear only source bytes 0–24, and
that a physical shop/guild row is shared. A detached `SHOP` v1 section now
round-trips all 1,440 physical records; it is **outside** the required known
save manifest until a live MM6 owner and refresh lifecycle are proved. Event
opcodes 20, 27 and 28 now advance as original no-ops. Indoor texture and
face-bit events target one checked face index for MM6; MM7 keeps its group
behavior. The MM6 changed-state writer does not save those face edits, and
event replay is not established on all affected maps, so reload fidelity and
campaign play remain open. Normal MM6 chest and SaveLoad guards stay closed.

The standalone `BEAC` v1 section now preserves current-runtime Lloyd beacon
images and an optional save thumbnail as exact RGBA bytes, keyed by lowercase
MM6 base-map filenames. Its linked focused tests passed **5/5**, the
private-backed MM6/LOD gate **336/336**, general non-MM6 unit gate **437/437**
and MM7 headless control **1/1**. The required known-save manifest still rejects
BEAC and active beacons; renderer-owned image publication, stable all-owner
commit and graphical save–relaunch–load remain open.

The observed original shop/guild shared edge also shares one refresh deadline:
the first service after expiry regenerates its stock and advances the deadline;
the other skips generation while it remains current. A detached MM6 refresh
helper stages both physical stock and clock owners before publishing either.
Its focused shop/clock suites passed **8/8**, and the private-backed MM6/LOD
gate passed **338/338**. Item generation, live house services and a nonzero
native-save comparison remain open.

Detached `BEAC` staging can now rebuild all saved beacon and thumbnail RGBA
bytes into owned CPU images before any renderer or Party mutation; its linked
focused suite passes **6/6**. The current viewport screenshot changes render
state and requires a stable game/render-thread capture point, while
`GraphicsImage::Create()` allocates GPU textures immediately. Renderer image
publication and the all-owner commit remain guarded.

The integrated synthetic indoor-face save/reload fixture passed **2/2** over
97 monitored headless samples without a visible window or focus change. The
current MM6 DLV writer emits unchanged bytes after face-bit and texture events;
base-map reconstruction loses those edits unless a reload event reapplies them.
Six of 17 affected retained maps have no reload trigger, so this is an explicit
MM6 save gate. A read-only shop access audit found the direct clear, generation,
display, purchase and raw-save paths but cannot exhaust indirect stock writers;
no nonzero owner-local native save or full 28-to-36-byte item conversion is
proved. Live face and SHOP persistence stay guarded.

The linked known-save fixture confirms that standalone `SHOP` and `BEAC` tags,
though recognized by the generic `.ec6` container, still reject from the
provisional eighteen-section assembly before destination mutation. A per-field
MM6 shop audit verifies the original 28-byte purchase copy and source clear,
but finds category-dependent unions, byte-26 meaning and absent expiration
bytes that prevent a reversible conversion to inherited `Item_MM7`. Raw stock
records remain the authority. The beacon renderer audit also found unchecked
GPU texture creation and potentially allocating deferred deletion, so CPU
pixel staging does not yet authorize live image publication.

A detached MM6 `IFOV` section now retains changed indoor face attributes and
texture identities by canonical map filename, with a fingerprint of the
deterministic pre-reload face base. It applies saved changed fields after
`OnMapReload` and rejects a changed base before mutation. Its integrated focused
tests passed **3/3**; a fourth linked test confirms IFOV, SHOP and BEAC remain
outside the partial known-save manifest. Live texture capture, renderer
publication and whole-save wiring remain guarded. A checked renderer contract
is documented, but no generic renderer patch was made.

The detached MM6 `SHOP` owner now uses an exact `ItemRecord28` value type with
little-endian structural accessors; linked item/shop/clock tests passed
**10/10** and the private-backed MM6/LOD gate passed **345/345**. This does not
make carried, held, ground or chest runtime items lossless. The provisional
`ITEM` and `HELD` logical `.ec6` sections derive from inherited `Item` and omit
native MM6 trailing bytes, so the final save manifest needs raw item owners
and context-specific adapters before live MM6 SaveLoad opens.

The detached `INVR` v1 section now preserves four characters' 138 physical
28-byte item records, 126 grid words and 16 equipment words each, following a
pinned MMExtension MM6 layout. Focused checks passed **5/5**, the private-backed
MM6/LOD gate **347/347** and general non-MM6 unit gate **437/437**. It is outside
the provisional known-save manifest and has no live raw Party owner. A read-only
trace of the accepted original executable confirms 138-slot allocation. MM6
party zeroing, including reset, now gives each character 138 runtime slots;
MM7 stays at 126. A
monitored headless test placed an item in the final MM6 slot and verified that
the inherited MM7 snapshot rejects this inventory before writing. The full MM6
headless suite passed **51/51** over 1,513 monitored samples without a visible
window or focus change, and
the MM7 control passed **1/1**. Provisional ITEM v1 can now save the extra
logical slots, but still loses native raw bytes, so normal MM6 SaveLoad remains
guarded.

The inherited MM7 party and character serializers now reject MM6 party or
138-slot character state at their entry points before destination mutation;
the direct inventory reconstruction path has the same capacity guard. The
updated focused test checks sentinels after rejection. The full MM6 headless
suite passed **51/51** over 1,504 monitored samples, an MM7 save/load control
passed **1/1**, and rebuilt non-MM6-prefix unit checks passed **438/438**.

An opt-in, detached [plain-item projection](MM6_F4_PLAIN_ITEM_PROJECTION.md)
now reads MM6 item-table types for zero-bonus records and gold while retaining
all 28 original bytes. It rejects unmapped bonus unions and has no live owner.
Linked raw-item/inventory/projection checks passed **6/6**; a monitored MM6
headless loaded-table category check passed **1/1** without a visible window
or focus change. This does not make any item-bearing MM6 save route playable.

The earlier detached [plain inventory projection](MM6_F4_PLAIN_INVENTORY_PROJECTION.md)
staged one physical 138-slot MM6 character region with exact grid and
equipment replay, retaining unreferenced raw records. Linked focused checks
passed **8/8**; a monitored loaded-table MM6 headless footprint check passed
**1/1** over 75 samples. Its API is superseded by the bounded stage below.

The [standard item bonus map](MM6_F4_STANDARD_BONUS_MAP.md) now supplies the
same source-backed 14-index mapping to the MM6 item-table loader and future
raw-item adapters. The private-backed MM6/LOD suite passed **352/352** and
rebuilt non-MM6-prefix unit checks **438/438**. A loaded-table MM6 headless
control passed **1/1** over 64 samples with no visible window or focus change.
Live nonzero bonus use is still gated on effect and strength bounds.

A later [detached standard-item projection](MM6_F4_STANDARD_ITEM_PROJECTION.md)
can stage source-typed MM6 equipment with a mapped standard bonus and positive
strength while preserving the exact 28-byte raw record. Focused map/projection
unit checks passed **3/3** and a monitored loaded-armor MM6 headless check
**1/1** over 65 samples without a visible window or focus change. It has not
been attached to a live inventory or verified against native nonzero saved
items, armor effects, mods or normal SaveLoad.

The current [bounded inventory projection](MM6_F4_BOUNDED_INVENTORY_PROJECTION.md)
combines plain/gold and standard-enchanted equipment views for one physical
138-slot character region while preserving every raw byte. Focused linked
checks passed **7/7** and two monitored loaded-table MM6 headless checks
**2/2** over 65 samples with no visible window or focus change. It remains
detached from Party and normal SaveLoad; creation/removal and equipment
changes still reject.

A [four-character bounded stage](MM6_F4_BOUNDED_PARTY_INVENTORY.md) now composes
those views into an all-or-nothing `RawInventoryFields` capture. Focused linked
checks passed **11/11**, including a late fourth-character rejection and an
`INVR` codec round trip. Live inventory moves copy `Item` values between slots,
so a Party-slot-only raw sidecar would lose provenance. The stage remains
detached; no normal MM6 save route is open.

The [item value provenance carrier](MM6_F4_ITEM_VALUE_PROVENANCE.md) now keeps
the 28-byte MM6 source record with staged plain and common standard items
through shared `Item` copies. Both bounded adapters reject absent or replaced
carriers; MM7 reconstruction clears it. Linked focused checks passed **13/13**,
general non-MM6 units **438/438**, an MM7 headless save/load control **1/1**
and two loaded-table MM6 headless checks **2/2**, with no visible window or
focus change. Physical slot mutation, unique item identity and final save
ownership remain open.

A read-only [raw transfer audit](MM6_F4_RAW_ITEM_TRANSFER_SEMANTICS.md) proves
specific first-25-byte backpack/cursor clears, full incoming cursor/shop
copies and zero-ID allocation across all 138 physical slots. Equipment
removal, general UI pickup and ground fallback remain unproved, so the
detached carrier cannot yet drive live inventory movement or SaveLoad.

The latest MM6 [NPC save-field correction](MM6_EC6_NPC_FIELDS_F4.md) preserves
the source `-1` no-house value copied during party creation. A fresh-process
headless check now writes a temporary `.ec6`, publishes the 67-map cache and
CHST state, and enters New Sorpigal through `DoPrepareWorld` with actors and
twenty chests (**1/1**, no visible window or focus change). The prior scoped
save rollback check still passes **1/1**, and focused NPC codec tests pass
**5/5**. This does not open the normal MM6 new-game/save/load route or prove
late world-failure rollback, durable reload or campaign playability.

A further isolated fresh-process GameTest now stages a persisted partial `.ec6`
from a disposable profile, publishes represented Party/NPC/timer/overlay/map
cache/chest owners in that test process, and re-enters the first outdoor map
through `DoPrepareWorld` with actors and twenty chests. The rebuilt monitored
probe passed **1/1** without a visible window or focus change; disk bytes stayed
unchanged. Staging uses the loaded MM6 NPC table as the base because static
name pools are not saved. This is not production `loadGame()`, a complete-owner
transaction, rendered-state proof or playable campaign. Provisional `ITEM` v1
now supports the existing 138-slot MM6 runtime inventory, including index 137
from a fresh 126-slot Party; focused durable-field tests passed **5/5** and
related save tests **31/31**. Native item bytes and slot transfer semantics
remain outside that provisional codec.

Three further MM6 F4 primitives are integrated: a title-local raw SHOP/SCLK
process owner with detached joint stage and no-throw publication; a bounded
four-character backpack item-event removal that clears physical bytes 0–24
and occupied grid cells while preserving opaque tails; and a detached 127×127
outdoor tile-ID projection that shares its local-to-global mapping with the
live terrain loader. Focused linked world/item/shop tests passed **18/18**.
Separate hidden processes wrote a temporary `.ec6` and re-entered the first
outdoor world from it (**1/1** each, no visible window or focus change). None
of these changes opens production house/event/save/load paths or proves a
failure-atomic outdoor/resource commit. The temporary profile was removed.
An MM7 all-map reconstruction control passed **1/1** after the shared terrain
mapping extraction, also without a visible window or focus change.

The first MM6 live chest entry now uses the title-local staged open-only
transaction. A dedicated headless first-map run opened an untrapped chest,
updated its GUI under NullRenderer and verified its raw sidecar and runtime
chest publication (**1/1**, zero visible windows and no focus change). MM6
traps and pickup remain guarded; production map entry and SaveLoad remain
closed. This does not establish rendered chest pixels or campaign play.

The next integrated linked save/preflight gate passed **31/31**, including the
new detached archive-backed New Game precommit case and separate EC6 v2 raw
inventory/held/shop candidate. Two fresh monitored MM6 processes wrote a
disposable partial New Game slot and reentered its first outdoor world; the
second advanced timer, actor AI, queued turn/walk and draw (**1/1** each,
zero visible windows and no focus change). The disposable profile was removed.
An MM7 all-map reconstruction control passed **1/1**. EC6 v2 remains offline;
normal MM6 SaveLoad and all-owner world publication are still guarded.

A hidden OpenGL first-map test also captured an item-bearing MM6 chest after
the title-local live open transaction. The monitored run passed **1/1** over
118 samples with no visible window or focus change; visual inspection found
the chest panel and item icons in the expected region. Its temporary PNG is
untracked. Live trap, pickup, persistence and campaign play remain guarded.

The latest fresh-process reload check passed a second monitored two-process
sequence (**1/1** per process) and covered the first world-update skip, then
the input skip with queued actions held, then a movement/draw frame that drained
them. A separate scoped combat-transient reset now stages four pre-load
containers and passed **3/3** linked rollback/commit tests. Neither is wired
into production `loadGame()` or a complete `Game::gameLoop()` handoff.

The combat scope now also snapshots seven scalar fields of the turn engine
alongside its queue. The revised integrated linked fault/commit suite passed
**3/3**. Persistent events, outdoor resource/listener state and other live
owners still prevent an all-owner load transaction.

EC6 v2 now stages raw inventory, held-cursor and shop owners only after a
complete 67-map/base and active-sidecar validation. The integrated focused
save/schema/map check passed **11 tests**; one optional private fixture test
skipped because its paths were not supplied. V2 remains detached and is not
used by the production MM6 SaveLoad path.

The native `.ec6` disk helper now has schema-isolated v1 and v2 write, list,
preflight and recovery APIs. The linked atomic-file suite passed **7/7**,
including both cross-schema overwrite directions and v2 injected write
failures. Structural v2 slot validity does not prove a playable save; normal
MM6 SaveLoad and menu listing remain guarded.

The detached v2 Party inventory adapter now stages supported plain and
standard equipment across all four characters, preserving native record
tails; nonzero potion, special and artifact unions still reject. A separate
first-map rollback scope covers the 75 map and 125 decoration event-variable
bytes. The integrated linked disk/inventory/event suite passed **13/13**.
Neither adapter is wired into a production MM6 load. A source audit proposes
one MM6 campaign per child process to avoid rollback to a prior live session;
it still requires a complete v2 candidate, durable saves and a checked first
frame. See [the handoff audit](MM6_F4_CHILD_PROCESS_GAME_LOOP_HANDOFF.md).

The v2 disk selector now feeds the detached raw map/item/owner stage without
repairing a selected backup. A linked synthetic test rejected a semantically
invalid primary and selected the valid backup with both files unchanged; the
raw-runtime plus atomic-file suite passed **11/11**. The result is not a full
Party/world/UI candidate and cannot enter normal MM6 SaveLoad or the menu.

A separate disposable v1 test now reaches the real MM6 Main Menu QuickLoad to
`Game::loop()`/`gameLoop()` load route through explicit test-only callbacks.
Fresh writer and loader processes passed **1/1** each under the host monitor
with no visible window or focus change; the loader checked first-map entry,
movement and unchanged slot bytes. The MM7 all-map headless control passed
**1/1** afterward. Production MM6 loading and menu slots stay guarded, and
this is not a complete v2 load or graphical campaign check.

Decoded v2 core and progress fields now stage on a detached Party through
adapters shared with the v1 route. Linked core/durable/raw-runtime tests passed
**17/17**, including direct-vs-v1 equivalence and invalid-field rejection.
The full v2 Party coordinator and live MM6 save/load remain open.

The bounded raw item adapter now includes MM6 Antique ID 58 when the loaded
MM6 table confirms its value-only special contract. Integrated linked item,
core and durable tests passed **23/23** with exact native record retention and
late-failure rejection. Other special unions, potions and artifacts remain
guarded; original-game effect comparison and live save/reload are still open.

The v2 raw stage now composes decoded core, progress and supported physical
inventory on one detached Party, rejecting divergent inventory images and a
late unsupported fourth-character item before return. Focused linked raw
runtime, inventory and Antique tests passed **11/11**. This is still a partial
Party; normal MM6 SaveLoad and Atlas Play remain guarded.

The v2 raw map stage now retains active actor/sprite/chest counts and feeds a
detached NPC, 67-map-buffer, timer, overlay and CHST stage shared with v1 field
logic. Linked raw-runtime/world-runtime tests passed **14/14**, including a
late invalid actor overlay that left input NPC/timer untouched. Live map,
renderer and UI publication remain unresolved.

Decoded v2 `RSTS` resistance and `BUFF` fields now have Party-copy adapters
shared with the v1 byte route. The integrated linked codec, adapter, raw and
world set passed **24/24**. A complete v2 session remains open.

Decoded v2 `HIRE` and `RCVR` adapters now join the other represented Party
fields on one detached copy. Focused raw/effect/hireling-recovery checks passed
**12/12**, including late recovery-clock rejection without baseline mutation.
The [full-session owner-gap packet](MM6_F4_V2_FULL_SESSION_OWNER_GAP.md) now
separates this Party stage from NPC/map/timer/CHST staging and lists the
remaining BEAC/IFOV, cursor/shop, active-world and UI admission gates.

A read-only v2 session-field coordinator now selects the native primary or
backup only after represented Party and NPC/map/timer/CHST staging succeeds.
The linked atomic/raw/effect/hireling set passed **20/20**. One structurally
valid primary with an unsupported late item fell back to its valid backup;
both-invalid candidates refused, and disk bytes stayed unchanged. This does
not admit a playable slot or open normal MM6 loading.

The MM6-only one-shot post-world test hook now supports a deliberate return to
Main Menu immediately after `DoPrepareWorld(true)`, before timer unpause or
input. Its linked hook suite passed **5/5**. In separate disposable headless
processes, a v1 first-map writer and a real Main Menu QuickLoad abort control
passed **1/1** each, with the slot unchanged; an MM7 headless control passed
**1/1**. This verifies the abort edge only. The v2 callback still refuses
before publication, and normal MM6 SaveLoad, menu loading and Atlas Play remain
guarded. See [the post-world abort gate](MM6_F4_POST_WORLD_ABORT_GATE.md).

A new read-only v2 [post-world owner validator](MM6_F4_POST_WORLD_OWNER_VALIDATOR.md)
compares final map identity, actor/sprite PID slots, persistent event bytes and
generated chest/sidecar state against the detached selected candidate. Its
three synthetic indoor/outdoor tests passed, and the combined linked
post-world hook/validator set passed **8/8** after integration. It is not
connected to the v2 QuickLoad callback; the remaining NPC, renderer/UI,
BEAC/IFOV, action and recovery owners still block live publication.

The final generated-NPC link check now rejects broken actor to generated-row
links, pool overrun and malformed rows without mutating them. Its integrated
synthetic suite passed **3/3**. This proves link shape only: v2 does not save
generated NPC identities or game RNG. The MM6 loader now refuses a map that
would overrun its 100-row pool before the first generated-row write; the
linked NPC suite passed **4/4**, and loaded MM6 first-map plus MM7 headless
controls passed **1/1** each. A separate v2
admission guard identifies nonzero saved Armageddon timers that world
preparation would clear; the linked child-publication suite passed **3/3**.
Neither check opens MM6 SaveLoad or first-frame input.

The empty-cursor post-world check now validates the actual MM6 mouse after
world preparation in native-arrow or custom-bitmap mode; the latter decodes
nonempty CPU pixels. The focused cursor suite passed **5/5**, the disposable
v1 writer and Main Menu QuickLoad abort processes passed **1/1** each, and an
MM7 headless control passed **1/1**. Occupied item offset, GPU presentation
and complete v2 publication remain open. A read-only
[menu preflight list](MM6_F4_MENU_SLOT_PREFLIGHT.md) also passed its linked
atomic-file suite **8/8**, selecting only semantically accepted native
primary/backup candidates without disk mutation; the visible MM6 load menu
stays empty until a complete gameplay validator and loader exist.

A disposable native v2 first-map writer and fresh-child real Main Menu
QuickLoad control now pass **1/1** each. The child published only represented
owners in its test process, rebuilt the first outdoor map, verified the empty
cursor, and aborted before input with the source slot unchanged. The final
PID-slot validator refused as designed: the saved pre-entry DDM has an initial
respawn marker, so encounter spawning adds actors after its saved actor
section is loaded. The current changed-state writer cannot encode that added
actor count. See [the bootstrap/respawn gate](MM6_F4_RAW_V2_BOOTSTRAP_RESPAWN.md).
The detached GNPC identity capsule passes **3/3** linked checks, but it is
still outside the strict v2 manifest and does not yet make generated NPC or
game RNG state durable.

A detached versioned [gameplay RNG capsule](MM6_F4_GAMEPLAY_RNG_SAVE.md) now
captures and restores the selected MT or sequential engine without consuming
a draw. Together with GNPC, the focused linked suites passed **5/5** and an
MM7 headless control passed **1/1** after the shared RNG code changed. The
strict v2 manifest still rejects both extra sections, and restoring RNG only
after map events would be too late for generated identities or event effects.

A detached [outdoor actor append codec](MM6_F4_OUTDOOR_ACTOR_APPEND.md) now
accepts complete native 548-byte records for newly spawned slots, preserves
original DDM records and opaque suffix bytes, updates actor count and
respawn day, and re-decodes its result. Linked synthetic checks passed
**3/3**. The disposable v2 writer and guarded fresh-child QuickLoad still
passed **1/1** each. Full native record capture from runtime actors remains
open; this codec is not yet a durable post-entry save.

The [spawned-actor owner decision](MM6_F4_SPAWNED_ACTOR_OWNER_DECISION.md)
selects a versioned, title-specific EC6 runtime section candidate for new
exact-slot actors. The present 548-byte MM6 projection omits major native
regions and `SpawnEncounter` retains no complete native-record provenance.
This decision adds no live save/load route; the detached section must prove
complete runtime field round-trips and join a no-respawn fresh-child reload.

A detached [pre-event GNPC/RNG agreement helper](MM6_F4_PRE_EVENT_GNPC_REPLAY.md)
now validates the active map, selected RNG type, fresh NPC cursor and exact
actor-to-generated-row anchors without mutation; linked checks passed
**2/2**. `DoPrepareWorld` generates rows and consumes RNG before
`onMapLoad()` events, so the eventual v3 replay must branch around only that
generation loop, retain the subroutine's other work, and publish saved
GNPC/RNG before events. No live hook is installed.

A detached [SPAW v1 runtime actor section](MM6_F4_SPAWNED_ACTOR_SECTION.md)
now preserves exact contiguous slots, MM6 resistance values and runtime-only
fields while refusing unsupported item/buff states and narrowing. Its linked
suite passed **4/4**; a private-backed fresh-process first-map capture
encoded, decoded and re-encoded its spawned actors **1/1** using ramfs user
state, without writing a save. This is not yet a durable `.ec6` slot or a
fresh-child replay, and later actor mutations remain outside the admitted
subset.

The first detached [EC6 v3 manifest](MM6_F4_EC6_V3_MANIFEST.md) required
the nineteen v2 owners plus SPAW, GNPC and RNGS and passed **12/12** linked
structural checks. It was superseded by full-list ACTR in the same candidate
manifest. The current v3/v2/ACTR suites pass **11/11**; it validates each
new payload and the GNPC/HEAD map join, but does not bind ACTR's baseline
portion to native DDM actors, prove no-respawn replay, or expose a v3 atomic disk route.
V1/v2 remain strict and cannot be silently loaded as v3.

A [baseline actor save audit](MM6_F4_BASELINE_ACTOR_SAVE_AUDIT.md) found that
the DDM writer omits significant changed runtime fields for original actors.
A disposable full-list runtime capture refused `missing-mm6-resistance`
(**0/1**) because the native baseline projection lacks that optional field;
the spawned-only capture remains **1/1**. SPAW-only v3 is therefore a
structural candidate, not a durable gameplay schema. Exact baseline actor
provenance or an exhaustive refusal gate is required before live use.

The MM6 outdoor loader now restores each baseline actor's exact six native
resistance bytes after reconstruction; a focused two-actor override test
passed **1/1**. The full-list headless capture progressed beyond its
`missing-mm6-resistance` refusal and identified a zero descriptor on an
actor that can still act. The codec now round-trips that exact sentinel
while rejecting other invalid identities; its linked suite passed **5/5**.
The full-list first-map capture passed **1/1** without a profile write,
and an MM7 headless control passed **1/1** after the MM6-only loader change.
The [active-actor decision](MM6_F4_ACTIVE_ACTOR_OWNER_DECISION.md) calls for
full-list ACTR replacing SPAW in the candidate manifest. Actor items/buffs,
live replay and durable save–quit–reload remain open.

A detached [ACTR v1 full-list section](MM6_F4_ACTIVE_ACTOR_SECTION.md) now
uses a distinct tag and bounded runtime records for every active actor slot
from zero. Linked ACTR/SPAW unit suites passed **8/8** and a dedicated
private-backed first-map ACTR capture passed **1/1** headlessly with no
profile write. The current v3 manifest now requires ACTR;
actor items/buffs, live pre-event replay and fresh-child save–quit–reload
remain open.

The detached [v3 atomic slot boundary](MM6_F4_EC6_V3_ATOMIC_BOUNDARY.md)
adds semantic-callback write and read-only preflight/list APIs, with both
primary and backup checked for recognizable foreign EC6 versions before
v3 fallback or mutation. The linked atomic suite passed **11/11** across
new v3 and existing v1/v2 cases. No production MM6 menu, backup repair or
live save/load route uses it yet.

A detached [v3 outdoor actor-map agreement](MM6_F4_EC6_V3_OUTDOOR_ACTOR_MAP.md)
now joins exact v3, the validated 67-map set, active HEAD/GNPC/CHST and
ACTR. Linked map/manifest/ACTR suites passed **10/10**. It requires a
matching nonzero DDM/sidecar day that will not respawn on load, stable
native actor anchors and an ACTR count within the runtime cap. Mutable
actor HP may differ from DDM; ACTR is the proposed owner. The helper is
read-only and does not establish live replay or first-input readiness.

A test-only [pre-event checkpoint](MM6_F4_PRE_EVENT_CHECKPOINT.md) now
runs after MM6 map/actor rebuild and before generated NPC creation or map
events. A scoped test-only ACTR/GNPC/RNGS replay now checks the staged
v3 owners and chest/active-DDM stability, publishes actors and generated
rows, bypasses only generated-row creation, and restores RNG before map
events. Linked replay/checkpoint checks passed **7/7**. A disposable
native v2 writer and separate real Main Menu QuickLoad abort child passed
**1/1** each after integration; the child still aborts before timer/input
with its slot unchanged. An MM7 headless control passed **1/1**. No
production v3 load caller or first-input admission is enabled.

A detached [v3 full-session preflight](MM6_F4_EC6_V3_FULL_SESSION_PREFLIGHT.md)
joins exact ACTR/map/CHST with represented Party/NPC/timer/event/
inventory/cursor/shop owners, pre-event GNPC/RNG agreement and a post-world
Armageddon plan. Its composed suite passed **8/8**, and a later linked
ACTR/map/full-session regression group passed **9/9**. No live owner or
slot is mutated.

The [v3 post-entry map and pre-event seam](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md)
now patches only supported DDM reset, visibility and sprite header fields,
retaining actor/chest/opaque bytes. A fractional sprite position is accepted
only when exact reconstruction and the loader's floor step reproduce the
live vector. The private-backed first-map retained-DDM/live-owner headless
probe passed **1/1**, superseding its earlier `sprite-header` and
`sprite-position-loss` refusals. This does not write a v3 slot or prove
later maps and mutations. A separate no-write full-session first-map
candidate passed detached `stageV3FullSessionPreflight()`. The event
classifier confirmed exact native 200-byte event data and zero generic
`mapVars`, then refused because inherited generic `decorVars` were nonzero
after map load: `event-decor-bound-high` (**0/1**). The nonzero value is
bound to an interactive decoration's assigned high `eventVarId`, rather
than an unbound transient. This supersedes earlier `event-legacy-decor`,
`map-preflight` and broad `persistent-event-join` classifications. Initial
respawn calls `RespawnGlobalDecorations()` before `PrepareDecorations()`,
filling generic `decorVars[0..123]`; the stamped no-respawn DDM reload
skips that call. Some event draws consume RNG. No exact mapping to native
200 event bytes is proved, and zeroing the generic owner would lose state.
A disposable v2 writer passed **1/1** as the control. The candidate guards
unrepresented commerce and cursor state and attempts
no atomic write. Writer-side complete session joins, BEAC/IFOV
policy, actor item/buff coverage, fresh-child v3 replay, final post-world
owner checks and production SaveLoad/Atlas admission remain open.
A disabled strict-input-only v3 fresh-child replay GameTest is present but
has not run; it requires a separately admitted disposable v3 slot.

A detached [EC6 v4 EVRT owner](MM6_F4_EC6_V4_EVENT_RUNTIME.md) now retains
per-map exact 75-byte `mapVars` and 125-byte `decorVars` arrays with native
respawn count/day anchors and source-order interactive-decoration bindings.
EVRT inner payload **v2** now includes exact position bits, yaw and trigger
range in each binding; the unpublished payload v1 refuses. A focused test
rejects same-type decorations swapped in position.
It leaves native DDM/DLV 200-byte event data independent in `MAPS`. The
exact v4 container requires v3's 22 owners plus `EVRT` (23 sections); v1–v3
decoders remain strict. A test-only MM6 restore scope validates no-respawn
and exact map/header/bindings before copying arrays ahead of outdoor or
indoor decoration preparation, with a pre-input abort on armed mismatch.
Linked focused container/codec/restore checks passed **12/12** and an MM7
headless control passed **1/1**. A detached outdoor-only v4 full-session
stage now joins active EVRT map/header/bindings with represented v3 owners
and caller-supplied rebuilt decorations. Its linked positive and refusal
suite passed **5/5** after heap-backed preflight staging. A strict opt-in
three-process first-map sequence now passes: disposable v2 seed **1/1**,
atomic v4 candidate writer **1/1**, and fresh-child Main Menu QuickLoad
replay **1/1**. The reader checks source-backed decoration flags, EVRT arrays,
native event bytes, ACTR/GNPC/RNGS, chest item provenance and represented
session owners, then aborts before input. The v2 primary and v4 candidate
remain unchanged. Focused v4 full-session/atomic checks passed **8/8**,
chest route checks passed **8** with one private-reference skip, and an MM7
headless control passed **1/1**. A separate strict fresh-child test continued
the verified reload into the real null-renderer game loop, reached first
input, turned and walked once, then quit with both slots unchanged (**1/1**).
This is a test-only first-map reload and bounded input boundary. A post-action
second write, indoor/later-map replay, production SaveLoad and Atlas remain
closed.

The first attempted post-action v4 writer refused before a second slot was
written: a gameplay-updated ground sprite had fractional X/Y coordinates
that the integer native DDM sprite record cannot preserve. The v4 guard is
retained. An exact full-list runtime sprite owner, bound to native slots and
MM6 raw item provenance, was needed for a later schema; waiting for
sprites to settle is not a general save strategy. See the
[v5 sprite-runtime gate](MM6_F4_EC6_V5_SPRITE_RUNTIME.md) for the implemented
test-only path.
Detached v5 SPRT/ARTR/DECR full-list codecs, exact 26-section manifest,
test-only pre-event scopes and atomic candidate shell pass **16/16** focused
checks. After a source-backed first-124 decoration ordinal fix, the strict
disposable v5 post-action writer passed **1/1** following v2 seed and v4
baseline controls (**1/1** each), preserving those source slots. The
first fresh-child v5 reader crashed before an owner result from large nested
stack frames. Heap-backed stage and PART decode fixed that test path. A
fresh four-process v2 seed, v4 baseline, v5 post-action writer and v5 reader
sequence passed **1/1** at each step; the reader checked represented owners
after world preparation and aborted before input. Separate strict v5 first
input, second-write and second fresh-child reader tests passed **1/1** each,
completing a seven-process disposable first-map cycle. The first-input test
turned and walked in the real game loop; the second writer required changed
position and yaw, wrote a separate atomic slot and kept v2/v4/prior-v5 bytes
unchanged; the second reader checked exact represented owners and aborted
before input. Production SaveLoad, indoor/later-map replay and Atlas remain
closed.

The next MM6 integration checks remain bounded. The normal manual and quick
save entry points now refuse unsupported MM6 saves before allocating a slot,
advancing the quick-save counter or reporting success; a headless check passed
**1/1**. This is a safety guard, not production `.ec6` writing. Active MM6
skill-event Compare and Set operations now use a title-specific 31-skill
adapter, including Learning; focused linked checks passed **15/15**, a
headless interpreter scenario **1/1**, and an MM7 headless control **1/1**.
Add/Subtract and invalid or Grandmaster Set values remain refused.

For the next map boundary, read-only headless checks decoded all **15**
shipped outdoor base/delta pairs. V5's detached replay preflight now admits
those exact outdoor identities without changing older schema rules; focused
checks passed **3/3**. A direct second-outdoor-map load exposed a wrong MM6
encounter-table column: the map parser used display labels where the engine
requires authored monster stems. The MM6-only column fix and a synthetic
table check passed **1/1**, and the seven-process first-map v5 chain still
passed. The direct second-map world load then passed in a hidden headless
process (**1/1**), and the reachability-aware loaded-table check passed
across all **67** maps (**1/1**). The one authored encounter family without a
ranked monster is unreachable by both random chance and decoded spawn
references; the resolver remains strict. A test-only oute2 atomic slot passed
isolation checks, then a fresh v2 seed, v4 baseline and strict oute2 v5 writer
passed **1/1** each. The writer accepted one separate candidate after exact
native decoration row-0 and static-derived actor-name handling; focused checks
passed **7/7**. A fourth fresh child now replays that candidate and checks its
pre-event and post-world owners, then aborts before input; all four children
passed **1/1** each. The v5-only final PID check binds a generated runtime NPC
link to its native zero link, GNPC row and ARTR owner while keeping native
actor and sprite slot anchors exact. Three further children passed a bounded
real-loop turn/walk with changed position/yaw, a distinct atomic second v5
write, and an independent fresh-child replay of that second slot, each
**1/1**. The full seven-process chain kept earlier slots byte-identical;
focused atomic checks passed **18/18**. Actual campaign travel and production
SaveLoad remain open.

A separate disposable v2 seed, v4 baseline and fresh-child location-change
test passed **1/1 per child**. It used a single exact MM6-only test intercept
for cross-map autosave, then completed the real location-change state and
target `PrepareWorld`, checked the target owner/cache graph, and applied
bounded input. Every disposable save file stayed byte-identical; intercept
units passed **3/3**. MM6 clears derived chest positions before reconstructing
a different outdoor map, preventing a stale source-map position from failing
the strict target chest join. Border routing, travel confirmation and normal
cross-map autosave remain guarded.

A detached MM6 border calculator now returns a bounded neighboring grid map,
wrapped arrival coordinates and hireling-adjusted travel time without reading
or changing Party state. A separate pure MM6 predicate models the source-backed
three-bit offer mask and zero-flying confirmation check; its live offer remains
unconditionally false while later confirmation, corner acceptance and durable
autosave remain unproved. Focused route, eligibility and intercept units
passed **11/11**.

The [border autosave transaction audit](MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md)
identifies the disposable-only v5 writer, paused-dialogue capture mismatch,
separate foot-confirmation autosave path and missing visited-map runtime
ownership. A production old-map checkpoint and failure policy are required
before enabling a route; the audit made no runtime change.

For indoor save design, a detached IFOV writer and fresh-child reader passed
against retained private BLV/DLV data, and a separate headless map-load check
captured the real face baseline immediately before `OnMapReload` (**1/1**).
These checks do not publish a live indoor save. A strict, detached indoor v6
shell now stages raw v2 with native owner anchors and optional IFOV while
explicitly refusing gameplay admission; its focused container/stage/atomic
suite passed **25/25**. A fresh v2 seed and separate real-data indoor v6 stage
then passed **1/1** each headlessly: the stage built an in-memory candidate
from a loaded BLV/DLV and pre-event face baseline, verified native owner
anchors, kept the seed bytes unchanged and refused gameplay admission. A
later three-process disposable sequence passed v2 seed, atomic indoor v6
writer and independent fresh-child reader at **1/1** each. The reader rebuilt
the DLV/pre-event face baseline and re-staged the exact candidate; it still
refuses gameplay admission. Live indoor owner publication and first input
remain open. The first-map v5 no-write capture was extracted into an engine
service and the corrected seven-process disposable chain passed again, each
child **1/1**. The installed MM7 helper and its profile remain untouched.

The current indoor v6 manifest also requires ordered IDRT decoration runtime
state, bringing it to 26 sections without IFOV or 27 with IFOV. A test-only
MM6 pre-event hook applied exact staged decoration fields before map events,
and the fresh-child reader recaptured them after reload. A disposable v2
seed, v6 writer and reader passed **1/1 each**, and focused linked v6 checks
passed **7/7**. The reader stages exact candidate chest state under a scoped
test gate; normal MM6 chest loading and SaveLoad remain guarded. Actor/sprite
publication, indoor first input and graphical acceptance remain open.

The indoor v6 manifest now also requires exact full-list `ISRT` sprite
runtime ownership, taking the required count to 27 without IFOV or 28 with
IFOV. It preserves exact ISCR native prefix records, source-free slot reuse,
appended runtime sprites and MM6 item provenance with a v6-only scalar
overlay. Focused v5 sprite/v6 indoor units passed **12/12** and the
disposable seed/writer/fresh-child reader passed **1/1 each**. A later
MM6-only pre-event scope published the full saved sprite list and recaptured
exact ISRT after map events; focused v6 units passed **10/10** and the same
three-process chain passed **1/1 each**. The new required IART indoor actor
runtime section retains names and binds native actor identity; focused v5
actor/v6 checks passed **14/14**. The first pre-event fixture could not
prove post-world equality because normal map events generate NPC links.
A separate post-world writer and fresh-child reader now pass exact GNPC row
bytes, gameplay RNG and IART/ISRT/IDRT recapture before input. Both disposable
three-process chains pass **1/1 per child**, and focused v6/GNPC/v3 units
pass **18/18**. The candidate still reuses seed Party/title owners rather
than a complete live indoor session; gameplay admission stays false and
production SaveLoad remains guarded.

A separate strict indoor v6 live-session mode now captures the represented
raw Party/title/NPC/inventory/cursor/shop/chest/metadata/map owners together
after a real indoor world load. A disposable seed, first-map v4 writer,
indoor v6 writer and fresh-child reader passed **1/1 each** in four headless
processes. The reader recaptures all 19 raw sections and
GNPC/RNG/EVRT/IART/ISRT/IDRT/IFOV exactly after map load. Native residual
checks still require exact chest provenance and door geometry outside the
loader's known face-delta repair. This is one indoor map with a null renderer,
no first input and no production SaveLoad; gameplay admission remains false.

As of 2026-11-07, the later [v6 indoor input packet](MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md)
supersedes that first-map-only status: two distinct indoor source cases each
passed a five-process, headless seed, live writer, bounded input writer and
fresh-child post-input replay chain. MM6 indoor actor resistance restoration,
empty decoration handling and test-only door-motion/visibility capture were
verified with focused checks and an MM7 control. Production MM6 SaveLoad,
general indoor maps, graphical play and Atlas MM6 Play remain closed.

The [detached border precommit stage](MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md)
copies and clamps an old-map Party position without changing live state.
Its focused linked border suite passed **11/11**. It always refuses a disk
checkpoint; visited-map runtime ownership, durable write outcomes and live
border travel remain open.

The [second-outdoor no-write border candidate](MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md)
then passed a fresh three-process seed, v4 baseline and later-outdoor writer
chain (**1/1 each**). It checked an in-memory old-map Party copy and v5
owner agreement against a synthetic edge position without writing a border
slot or changing live Party core. Actual edge movement, visited-map ownership
and durable autosave remain gated.

The [all-source indoor sweep](MM6_F4_INDOOR_SOURCE_SWEEP.md) parsed and exactly
round-tripped every retained MM6 indoor BLV/default-DLV pair. A risk-ranked
additional case passes a third five-child test-only v6 replay chain. The
earlier fixed signed actor/follower interpretation is superseded by the
[original MM6 map-load trace](MM6_F4_NATIVE_SIGNED_NPC_MAP_LOAD.md): the
native DLV retains the signed marker, but the ineligible actor's runtime NPC
ID is cleared before gameplay. The prior pre-event follower refusal is a
transient diagnostic. All three chains, focused units and MM7 control pass.
Eligible StreetNPC generation, real dialogue/hire-fire checks,
production SaveLoad and general indoor play remain open.

The later [detached outdoor v6 visited-map work](MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md)
supersedes the missing-section part of the border notes above. Versioned
`VIST` holds a source-backed visited-map runtime owner beside the active v5
state. An owner-only atomic v6 writer and independent fresh-child reader
passed a five-process disposable headless chain, each child **1/1**. The
reader rebuilt the active map, checked the visited owner against its source,
and stopped before input without publishing that owner to the live cache.
Malformed and cross-map candidates refused, and an injected pre-replacement
fault left existing slots unchanged. A read-only return plan retained the
v6 active-snapshot chest/artifact ledger, which may be stale after further
play; a future return must carry the latest global ledger. A detached arrival
helper then staged a
copied Party across a cardinal edge, sampled target terrain, and refused
model-floor overlap; focused units passed **3/3** and the five-child chain
passed again. This did not execute real border movement or enter the old map.
`DoPrepareWorld(false)` replay, target-world agreement, rollback, durable
result policy, production SaveLoad and border travel remain closed.

An opt-in [pre-event return probe](MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md)
then entered the normal `DoPrepareWorld(false)` path for the visited outdoor
target. It compared the loaded target's terrain and floor against the
detached arrival, and aborted to Main Menu before live Party placement, chest
generation or map events. A scope unit passed **1/1**; fresh disposable seed,
v4 and return-probe children passed **1/1 each**. This establishes one
target-floor agreement, not a full visited-owner replay, rollback or
production return route.

The first-map v5 fresh-child post-world owner agreement has been extracted
from its GameTest callback into an MM6-only read-only engine service. The
test still owns publication scopes and the admission decision. After a
move-ownership guard was corrected, the established seven-process disposable
seed, v4, v5 write/replay, first input, second write/replay chain passed
**1/1 in every child**; an MM7 headless control passed **1/1**. This is a
preparation for a future first-map production save/load transaction, not an
admitted MM6 slot or normal menu action. Atomic result/recovery policy and
normal SaveLoad/UI routing remain open.

A test-only first-map v5 atomic outcome policy now distinguishes an exact
committed primary/backup state from a refusal before primary replacement and
an unconfirmed new primary after a later failure. Read-only preflight checks
the two existing disposable candidate slots, rejects foreign version or
wrong-map occupants, and selects a valid primary or backup without repair.
A post-primary-rename, pre-directory-sync fault was exercised with disk bytes
observed; focused linked units passed **2/2**, and the MM7 headless control
passed **1/1**. Normal MM6 SaveLoad and menu listing remain closed.

The detached first-map v5 source-backed load preflight has also moved from
the fresh-child GameTest override into an MM6-only engine service. It checks
the MM6 title and v5 first-map HEAD before materializing source decorations
and all generic v5 owner/source joins, then returns a detached graph without
changing Party, NPCs, map cache, disk or UI. Both first-map fresh-child tests
now call that service. The seven-process disposable chain again passed
**1/1 per child**, with an MM7 headless control **1/1**. Test-only publication
scopes, input admission, normal SaveLoad/UI and partial-world rollback remain
open work.

The first-map test-only post-world decision now fails closed: false or a
validation exception requests abort to Main Menu before timer unpause or
input. Focused one-shot units passed **6/6**; the fresh seven-process v5
chain still passed **1/1 per child**, with an MM7 headless control **1/1**.
This does not roll back the Party/NPC/timer/map-cache globals assigned before
world preparation. A normal load still needs a fully staged precommit bundle
and a safe partial-world failure policy before SaveLoad/UI can open.

A first-map v5 disposable writer service now stages from exact live MM6
owners, validates the detached candidate and prior slot, and classifies the
atomic result without opening a normal save path. Its focused writer/atomic
units passed **3/3**. The two first-map fresh-child readers now construct
all replay scopes before assigning Party/NPC/timer/map-cache globals, closing
the constructor-time partial-publication gap in those tests. The combined
fresh seven-process chain passed **1/1 per child**, and MM7 control **1/1**.
Global assignment and pre-event failures still need a safe transaction;
production SaveLoad/UI remains closed.

A pure first-map prepublication check now validates the detached staged
event boundary and cursor/shop join before either fresh-child reader
constructs replay scopes or assigns global owners. The linked seven-process
chain again passed **1/1 per child**, with an MM7 control **1/1**. The later
publication functions still revalidate, and global assignment or pre-event
replay can fail after mutation; these need a game-loop-owned abort/recovery
transaction. Manual Save pauses the timer before `doSavegame`; the later
paused-menu capture below supersedes the missing capture-contract part of
this status, but normal `doSavegame` remains guarded.

Main Menu Quick Load now refuses an ordinary MM6 request before probing a
present `.ec6` quicksave or queuing an unsupported load; the existing
test-only autosave override remains available, and MM7's branch is unchanged.
A focused headless test created a visible marker in GameTest's isolated user
filesystem and passed **1/1** with no pending load or menu transition; MM7
control passed **1/1**. The broader UI audit keeps MM6 load listing empty
until source-backed slot preflight, publication and failure recovery are
joined. A paused-menu capture alone does not admit a Save action.

An explicit MM6 first-map `PausedSaveMenu` capture contract now requires a
paused timer, the real idle Save screen and window, no text input or queued
UI message, and the same strict live-owner checks. A fresh disposable seed,
v4 and real Escape-to-Save-menu capture chain passed **1/1 per child** without
writing a slot; the existing unpaused seven-child v5 chain passed **7/7**
after this shared capture change, and MM7 control passed **1/1**. This proves
read-only capture at the paused menu, not normal `doSavegame` publication,
production slot policy or load recovery.

The paused-menu contract now feeds the existing **test-only**, owner-only v5
writer. In a fresh disposable profile, seed, v4, an existing v5 primary,
real Save-menu write with exact primary/backup checks, and independent
fresh-child replay each passed **1/1**. MM7 control passed **1/1**. This
proves a candidate written from the paused menu can be replayed, while
normal `doSavegame`, production slot naming and UI success state remain
closed.

A test-only armed MM6 first-map load scope now catches failures during the
load callback or world preparation in `Game::gameLoop`, clears pending load
and transition state, pauses the timer, and abandons the partial world to
Main Menu before input. Fresh disposable seed, v4 and v5 writer children
passed **1/1 each**, followed by injected partial-assignment and pre-event
fault children at **1/1 each**. A later stronger fixture completed a second
source-backed first-map load, exact post-world owner agreement and bounded
turn/walk after each abandoned attempt, again **1/1 for both fault cases**,
with save bytes unchanged.
The ordinary unarmed seven-child v5 chain still passed **7/7**, and MM7
control passed **1/1** after the shared game-loop change. This is not a
rollback: already published globals may remain unsafe. Normal MM6 load/UI
remains closed until a production recovery transaction is proved.

An MM6-only native first-map slot policy now stages an exact startup-supplied
profile root and numbered `saveNNN.ec6` identity, refusing foreign versions,
wrong maps, collisions and symlinks before test-only atomic writing. Focused
outcome/collision units passed **2/2**, with MM7 control **1/1**. The paused
live candidate is not yet connected to a numbered production slot or Save UI.

The Engine now owns the detached first-map v5 load graph and its exact
MM6/first-map/disposable-slot identity through a source-bound chest handoff.
The attempt advances from detached to published to chest-applied only after
native map and saved chest owners agree. The fresh seven-child replay chain
passed **7/7**, both full-retry fault children **1/1**, and MM7 control
**1/1**. Event, actor, NPC, RNG and post-world lifecycle still use test-only
hooks; this is not normal MM6 SaveLoad or production profile admission.
