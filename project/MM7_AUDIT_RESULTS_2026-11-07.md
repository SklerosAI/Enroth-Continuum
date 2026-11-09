# MM7 inherited-engine audit results — 2026-11-07

Gate reference: [plan §18.2](plan.md) and [MM7_AUDIT_CHECKLIST.md](MM7_AUDIT_CHECKLIST.md).
All results are static (source and data read offline) unless marked otherwise.
Detailed per-entry outputs live only in the ignored
`local-game-data/mm7/audit-2026-11-07/` and `local-game-data/private-wiki/mm7/AUDIT_*` notes.
This file holds IDs, counts and source references only, no game text.

Source revision audited: `0d94ac55a` on `mm6-mac-implementation`. Frozen MM7
baseline: `mm7-isolated-runtime` at `020cecf49`.

## A0 baseline (done)

Recorded privately in `A0-baseline.md`: integration and baseline revisions,
installed Atlas and `MM7` helper hashes, every MM7 data file hash, the
`Profiles/MM7-Atlas` settings and save hashes, and Atlas preferences.

## A1 data inventory (done)

Coverage, checked by offline parsers that mirror the engine readers:

- Archive entries manifested with size, decompressed size and SHA-256:
  BITMAPS 1614, ICONS 4241, SPRITES 9343, Events 198, GAMES 152, Audio.snd
  2519, Might7.vid 162, Magic7.vid 14, plus 19 loose music files and 2 `.hwl`.
- No exact or case-insensitive duplicate names (the LOD reader throws on
  duplicates). No overlapping regions or tail bytes. Every compressed image,
  sprite and pseudo-image ends exactly at its declared size. Every VID entry
  matches its container length.
- 36 text and 10 binary tables indexed and linked to parser and consumer.
  No out-of-range, duplicate or unparseable IDs. All `.bin` tables end exactly.
- All 76 mapstats rows pair with base, delta, EVT and STR, with no orphans.
  All 152 map files decode to their exact end.
- Shipped but unread for MM7: `Launcher.txt`, `dchest.bin` (chest data is
  hardcoded in `ChestTable.cpp`), 20 stale table copies inside ICONS, both
  `.hwl` files, two fonts. No file or entry the code expects is missing.

Defects, all inherited from upstream unless noted:

1. EVT SetSprite in D22 event 460, OUT02/OUT11/OUT14 event 250 and OUT03/OUT04
   event 500 names a decoration that no `ddeclist` entry has. The lookup
   returns `DECORATION_NULL` (`DecorationTable.cpp:11-15`, `Engine.cpp:1298`).
   Needs a runtime check of vanilla behavior, then a data alias or patch.
2. Audio.snd entry for sound 17000 (Light Bolt cast, `Spells.cpp:414`) has a
   zlib error at byte 6722. The best-effort fallback recovers about 7448 of
   9956 bytes, so the sound is likely truncated.
3. Two face textures referenced by out01 and t01 are absent, so those faces
   draw the PENDING placeholder (`ImageLoader.cpp:78-81`).
4. House 522 (reached from D33 event 376) has no shipped room video. It opens
   without one (`MediaPlayer.cpp` `tryOpenHouseMovie`).
5. NPC 348 (placed in `nwc`) has no portrait image.
6. About 40 house room sound IDs are absent, mostly the type-4 goodbye. Only a
   warning is logged.
7. Latent asserts: frame groups for monsters 259-261 and spell 78 variants
   point at missing sprites behind `assert(sprite)` (`Sprites.cpp:66/97/142`).
   No data references them.
8. 123 item IDs without icons (160-185, 272-299, 399, 499, 553-599, 671-672,
   781-799). None can be generated, found in a chest or given by EVT.
9. Text-table writes are not bounds-checked in Release (`IndexedArray.h:174`).
   Shipped data is in range, a bad mod table would corrupt memory.
10. Unchecked indexing in map loaders (`CompositeSnapshots.cpp:171-190, 221,
    356, 430, 441`, `EntitySnapshots.cpp:1606`). Shipped data never hits it.
11. Fork-introduced but harmless for MM7 data: `NPC_PROFESSION_LAST` 77,
    `HOUSE_LAST` 557, message-scroll base 500.

Tests that would close the open items: tape cog 56 after the SetSprite events,
tape the Light Bolt sound length, screenshot the out01/t01 faces, enter house
522 through D33, talk to NPC 348.

## A2a rules (done)

Static review against the upstream merge base `5b54e5758`. MM7 data only.
The top five defects were re-read in source by the auditor.

Fork gating: every shared rules file (Character, CastSpellInfo, Spells, Actor,
Monsters, Party, Item, Inventory, Chest, ItemTable, party creation, HUD,
popups, houses, map load) is MM6-gated, early-returns for other titles, or is
test-only. Remaining MM7 changes are opt-in mods (armor override, vendor
refresh, portrait stems) plus `NPC_PROFESSION_LAST` 58 to 77, which only grows
arrays.

Coverage:
- Spells: all 99 have a handler. Mana and recovery come from hardcoded
  `pSpellDatas` (`Spells.cpp:182-292`) and all 99 novice costs match the
  manual. Mastery minimums follow 4/3/3/1 in all nine schools. Scroll,
  spellbook (99/99) and wand (25) maps are correct. All 21 event-cast and 30
  monster spell IDs have handlers. No reachable `assert(false)` from content.
- Items: 799 items, 24 standard and 72 special enchantments, 618 random-item
  rows, the 50x50 potion table and 82 potion notes replicated. Equip gates, AC
  and damage multipliers, enchantments, artifacts 500-536, potions and mixing
  match the data and manual, apart from the defects below.
- Characters: race stats and resistances, HP/SP tables, mastery caps (1224
  cells, 9 differ), 27 promotion events, 99 teacher entries, 10 training caps,
  the XP table and 58 NPC professions checked.
- Monsters: 276 rows and all 39 columns mapped, hostility (89x89) and
  placement tables checked, 23 special attacks, flee, loot, party death and
  reputation hooks match. MM7 has no difficulty-dependent rules.

Defects, inherited unless noted:

1. Break Weapon is inverted (`Character.cpp:1624`, `if (!itemstobreaklist.empty()) return 0;`).
   Weapons never break, and an unarmed target reaches `randomSample` on an
   empty list, which asserts. Monsters 57, 80, 112, 207, 212. Upstream
   regression from `3d3802a8f`.
   **Fixed in the integration tree at `f77d8de31`:** MM7 now selects
   eligible weapons and returns safely for an unarmed target. The old MM6 path
   remains with Claude's MM6 rule lane.
2. Divine Intervention recovery (`CastSpellInfo.cpp:2679`) sets
   `5_ticks * spell_level` instead of subtracting it from the base, so higher
   skill lengthens recovery. Changed in `fdce3b207`.
   **Fixed in the integration tree at `f77d8de31`:** MM7 subtracts
   the skill reduction from its base recovery. The MM6 branch is unchanged.
3. Lich immunity to Mind and Body never fires (`Character.cpp:1258-1264`
   compares a `DamageType` cast to `Attribute`). The UI still shows immunity.
   **Fixed in the integration tree at `f77d8de31`:** MM7 liches take
   zero Mind and Body damage. A 2026-11-09 source-and-rule review confirmed
   that Spirit is intentionally not immune; the existing regression therefore
   correctly covers Mind and Body only.
4. Divine Restoration resets conditions on the drinker instead of the target
   (`Character.cpp:3561`).
   **Fixed in the integration tree at `f77d8de31`:** The potion clears
   the chosen target's removable conditions while preserving the drinker's.
5. Archer monsters never fire fire arrows: the parser has no case for that
   projectile token (`Monsters.cpp:151`). Upstream #2144.
   **Fixed in the integration tree at `f77d8de31`:** `FireAr` now
   decodes as a flaming arrow. A live archer firing check remains open.

6. Monster spell and projectile damage use the wrong ability slot
   (`Actor.cpp:274, 354`, `a4` ignored). Rows 18, 23, 24, 35, 36, 211-213.
   Meteor Shower always centres on the party.
   **Ability stamp fixed in the integration tree at `387b97a59`:** the selected
   spell slot now reaches projectile, Meteor Shower, Sparks and Sharpmetal
   sprites. Three MM7 GameTests failed before the fix and passed after it;
   three MM6 combat controls passed. Two affected trace fixtures were
   retraced on the combined source and remained byte-stable on repeat.
   Correct slot-two damage changed `Issue1972`'s total party HP loss from
   934 to 689; its independent check now requires at least 650 damage while
   its map and 24-meteor assertions remain. A 2026-11-09 review found the
   enemy Meteor Shower's party-centred position is its intended sole target,
   not an outstanding positioning defect.
7. Death explosions use attack-1 dice and type (`SpriteObject.cpp:825`,
   `Actor.cpp:1220`), and the parser drops the damage type. Nine rows.
   **Fixed in the integration tree at `b1760b19e`:** MM7 death explosions
   use their parsed special-damage dice and type; MM6 retains attack-1.
   The public Magog replay was retraced for the two newly consumed RNG draws
   and was byte-stable across three runs (`c81540e7d`).
8. Multi-target monster attacks are parsed but never applied. Ten rows.
   **Audit claim superseded on 2026-11-09:** `Shot x2` and `Shot x3` already
   route through `Actor::AI_RangedAttack` as the stacked and spread projectile
   patterns respectively. No runtime repair is warranted.
9. Fate is broken: the character buff is never read, and on a monster it
   raises the monster's own hit chance (`CastSpellInfo.cpp:1686`,
   `Actor.cpp:3707, 3727`).
   **Fixed in the integration tree at `6a9ea5261`:** MM7 character Fate
   applies its documented one-use hit bonus, while monster Fate subtracts its
   one-use hit chance. The MM6 actor behavior remains unchanged.
10. Artifacts from level-7 treasure are neither marked found nor capped
    (`Item.cpp:234-252`). Reachable via level-7 chests in d09, d12, d27.
    Upstream #1988.
    **Fixed in the integration tree on 2026-11-07:** MM7's level-7 picker now
    marks its selection and honors the configured artifact cap. An indoor
    spawned artifact retains its pickup item. Two focused MM7 GameTests passed,
    along with seven atomic-save tests and three MM7 controls (12/12 total);
    the MM6 startup/chest guard passed 1/1. The installed app and frozen MM7
    baseline have not been replaced, and a real chest pickup remains to inspect.
11. Smaller spell defects: the Day of the Gods event path is half-fixed
    (`Spells.cpp:800-815`, event 532 gives the wrong power), Spirit Lash
    drains mana out of range (`CastSpellInfo.cpp:946`), a scroll is consumed
    when targeting is cancelled (`Character.cpp:3691`), a wand charge is spent
    before the outcome (`Character.cpp:4654`), wand recovery uses the novice
    row for the cast but the expert row for display (`Character.cpp:1849`),
    the Meteor Shower damage formula changed in `a304b867b`.
    **Fixed in the integration tree at `fe8d5d50f`:** the MM7 event Master and
    Grandmaster Day of the Gods powers now match ordinary casts (`4 * level +
    10` and `5 * level + 10`); their 4-hour and 5-hour per-level durations
    remain. The level-10 test failed on both power values before repair and
    passed afterward. MM6 refuses this event source before the shared case.
    **Further fixed on 2026-11-09:** Spirit Lash now retains mana and uses
    failure recovery when its target is out of range (`be9a160bc`), and a
    charged wand uses the novice spell-recovery row consistently
    (`59ead390d`). Cancelled-scroll consumption and charge-spend ordering
    remain open.
    **Further fixed on 2026-11-09:** MM7 Squire contributes +2 to every
    weapon and armor skill, and Sage contributes +6 to Item ID and Monster ID
    (`dbd5dfba5`). These title-specific bonuses leave MM6 unchanged.
12. Monster buffs and targeting: monster Haste and Day of Protection are never
    read, the Hour of Power hit check reads the Shield buff's power
    (`Character.cpp:4549`), energy attacks deal Earth damage
    (`Monsters.cpp:113`), the target index in `_SelectTarget` never advances
    (`Actor.cpp:2089`), turn mode skips actor index 10 (`TurnEngine.cpp:136`),
    the indoor summon floor check looks like a decompile leftover
    (`Actor.cpp:2414`).
    **Energy parser fixed in the integration tree:** MM7 `Ener` in both monster
    attack fields now maps to Energy (12), while `Earth` remains Earth (3).
    The synthetic test failed on both `Ener` fields before repair and passed
    with Earth and MM6 parser controls afterward; the MM6 parser is separate.
13. Items and professions: two enchantment families add no elemental damage
    (`Item.cpp:71-130`), artifacts 530, 531 and 536 lack their described
    effects, the Squire profession does nothing and Sage lacks its identify
    bonus (`NPCEnums.h:73, 84`), two grandmaster teacher requirements differ
    from their text (`NPCTopics.cpp:519-527`), Archer and Warrior Mage
    Disarm Trap caps differ from the manual (`mm7_data.cpp:1450, 1491`).
14. Upstream issues confirmed still present: #2867 (zombie face persists,
    Divine Intervention is an extra path), #2502, #1463, #2770, #2763, #2786,
    #2071, #920/#2003, #1530, #682, #586, #1917, #1918, #843, #2268, #2232.
15. Stale or not applicable: #1485 (exists as `regen_stacking`), #1246, part
    of #496. #2982, #2278 and #2268 need a run.

The five branch defect tests failed on the preceding source. Six focused MM7
GameTests passed after the fixes, including the unarmed Break Weapon control.
Three MM6 combat children passed in separate processes. The integrated monster
ability stamp had three failing MM7 tests before its fix and passed 3/3 after
it, with three MM6 combat children passing. On the combined branch, the full
MM6 first-map gate passed its seven-child chain, retry fault children,
paperdoll, normal Save/Load, production load/resave and MM7 controls. The
full MM7 headless suite passed 467 tests with 2 skipped out of 469 under
network denial. Graphical acceptance and installed helper update remain open.

The later combined Energy and Day of the Gods source passed the full MM6
first-map gate and a network-denied MM7 headless suite with 469 passing and 2
skipped out of 471. The Energy correction caused `Issues.Issue403_970` to
diverge at a public trace RNG checkpoint. An energy-only build reproduced the
failure; temporarily restoring the old Earth mapping passed the focused test.
The corrected-source retrace changed only RNG checkpoints and was byte-stable
on repeat. The Shoals-to-Lincoln map transition and four Armor Class checks
remain in the test. The Day of the Gods fix and trace update are integrated;
no graphical or installed-app claim follows.

The branch GameTests cover the first five test targets above. Remaining tests
that would close open defects: rows 24 and 35 slot-2 spells, a level-7 chest artifact flag, the event 532
pedestal, a cancelled scroll, Squire and Sage
effects, the two teacher requirements, zombie clearing paths, and #2502.

## A2b campaign and events (done)

- All 77 scripts (76 maps plus `global.evt`) framed: 16,699 records, 47
  opcodes, 0 framing errors, 0 broken jump targets, 0 missing MoveToMap targets
  (271 records), 0 missing ShowMovie names (8 records).
- No shipped MM7 record uses a stubbed opcode (48, 52, 54, 55, 61-68), an
  MM6-only opcode (20, 27, 28) or InputString (26). Event 311, which asserts in
  `NPCTopics.cpp`, is in no topic slot.
- All 120 shipped (opcode, variable) pairs have a real case in
  `EvtVariables.cpp`. One operand fails the validity check, `global:47` item
  999, a known vanilla bug (upstream #673). The engine skips that step.
- NPC topics, SetNPCTopic targets and SpeakInHouse/MoveNPC house IDs resolve.
  Every referenced house type routes to a UI class in `createHouseUI`.
- A static campaign graph covers the opening, Harmondale and the castle, the
  alignment split, both four-task chains, the final dungeon routes, both
  endings and 27 promotion events. Every gate uses handled opcodes and existing
  maps and movies.
- Every MM7 change in the fork's event, NPC, house, transport, shop and guild
  code is MM6-gated or behavior-identical, except the opt-in vendor refresh mod.

Risks, all inherited:

1. Neither ending is proven at runtime and no game test covers one. Path:
   `global:170` or `global:172` checks item 605, plays the ending, then
   SpeakInHouse 600/601 opens the game-over window. On close the game
   autosaves and returns to Harmondale (`Game.cpp:902-917`).
2. Harmondale raid soft-lock risk (upstream #1665): qbit 183 locks the
   Harmondale doors until every group-9 raider in d29 and out02 is dead.
3. The arbiter messenger waits about 112 game days after the castle is rebuilt
   (`out02:110`, `out02:211`). Deliberate in vanilla, verify at runtime.
4. `d26:448` SpeakInHouse 440 points at an empty house row (type INVALID).
5. Known vanilla script bugs (upstream #673): qbits 40, 56 and 197 are read
   but never set. Oracle bits 229, 237 and 241 are never set. Bit-reset
   problems on 212, 213, 228 and 234 allow item duplication.
6. 73 face or decoration event IDs are referenced by map geometry but not
   defined in EVT. None are on the main chain.
7. Upstream issues to watch: #2778 (a movie replays on every exit), #2470 and
   #1808 (Thief promotion), #682 (quest items lost from full chests), #1829
   (barrow exit lands on the wrong side), #1560 and #2276 (crashes on optional
   maps).

Tests that would settle them: arbiter choice for light and dark, each ending,
the wetsuit edge, the final dungeon exit item check, the arbiter timer, raid
clearing, the two edge house clicks, and the opening move to Harmondale.

## A2c platform, saves and identity (done)

Independence (code reading): launched with Atlas arguments, MM7 reads only
`--data-path` and embedded resources and writes only inside `--user-path`.
Nothing reaches `~/.openenroth`, `Application Support/OpenEnroth` or MM6/MM8
roots. Outside the profile: OpenAL Soft reads its standard config files, and
macOS writes crash reports and saved state by bundle ID.

Defects:

1. **Fixed, fork build.** The earlier Release helper linked XQuartz
   `/opt/X11/lib/libGL.1.dylib`. The replacement Release helper uses the Apple
   OpenGL framework, has no `/opt/X11` link, and passed staged and installed
   code-signature checks. The installed Atlas launched MM7 to the title menu
   and returned normally on 2026-11-07. A future source-built dependency
   cutover remains separate.
2. **High for tests, medium for play, fork-introduced.** Commit `0b3b47e6d`
   moved the house pause after the `no_video` and decode checks
   (`src/Media/MediaPlayer.cpp`), undoing upstream PR #2956. MM7 houses stopped
   pausing time and music when the movie is off or missing. Fixed in
   `c11c2c1f9`; `Prs.Pr2956`, `Issues.Issue840` and `Prs.Pr2354` passed 3/3
   together after the follow-up MM7 changes. `Issues.Issue840` alone does not
   tape the pause.
3. Medium, M7-A6: the installed helper still statically links SDL3, FFmpeg,
   OpenAL, libpng and zlib from OpenEnroth-hosted prebuilt dependency releases,
   without their source notices. On 2026-11-07 the tracked-source recipe ran
   against integration source under network denial and staged a separate Atlas
   with all nine local archives and 23 media notices. Link-input and
   system-library checks passed; a bounded headless menu launch survived.
   The source-linked GameTest then passed the disposable fresh-process save
   and reload 1/1 each with byte-identical slot contents, plus focused MM7
   controls 3/3. Graphical play and save/reload remain before an installed-app
   cutover.
4. Medium, inherited: MM7 saves are not atomic (truncating `fopen "wb"`), a
   failed manual save or quicksave throws out of `saveGame` and ends the
   process, and a save with a valid header but corrupt body is listed and then
   crashes on load. Atlas crash reporting was corrected in `f70627f9c` and
   its abnormal-exit warning appeared after a controlled force-quit at the
   title menu. Ordinary Exit returned the normal message. Save failure and
   corrupt-body handling remain open.
5. Low-medium, fork: the spellbook Grandmaster popup row lost its `\t000`
   indent reset (`UIPopup.cpp:1415-1422`). Restored in `f70627f9c`; a visual
   comparison is still open.
6. Low, fork: a failed house movie reload called `Unload()`, unpausing the game
   inside a house. Fixed for MM7 together with item 2.
7. Low, fork: widened enum bounds (see A1 item 11), `SpawnEncounter` returns
   early on an empty name, a chest-ID assert became a bounds check.
8. Low: OpenEnroth identity left in the installed app: bundle icon names, the
   default config title, the log banner, the cheat overlay text and a stale
   legacy `openenroth.ini`. Attribution (license, credits, notices) must stay.
9. Low, inherited: `SDL_EVENT_MOUSE_WHEEL` falls through without `break`
   (`SdlEventLoop.cpp:78`), harmless.

Mac interface (code reading): trackpad touches are dropped safely, keys are
by keycode, focus loss pauses timers and audio, Cmd+Q exits with no autosave,
fullscreen uses an SDL Space on display 0, no HiDPI flag is set. Upstream
#1201 (HiDPI) and #2171 (window size after a fullscreen toggle) still apply.
#1920 is avoided by Atlas passing `--data-path`. #1591 (signing) still applies.

Remaining tests: corrupt-save load, unwritable save, spellbook popup screenshot,
and an explicit run with XQuartz unavailable.

## Follow-up after the A2 snapshot

The inherited `TurnEngine.cpp` actor-ID-10 skip in A2a item 12 was removed in
the integration candidate on 2026-11-07. A new real first-map actor test failed
before the one-line change because actor 10 was absent from the initial turn
queue, then passed after the change. The combined focused MM7 set passed 6/6,
and an MM6 startup/chest guard passed 1/1. This proves the initial queue case;
representative turn-based combat play remains open. Claude's MM6 worktrees did
not have changes in `TurnEngine.cpp` or the test file when this fix began.
