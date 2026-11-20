# MM6 campaign status

Where Might and Magic VI stands on `claude/mm6` at `a927bd004` (2026-11-19): what a player can do, how it is verified,
the campaign chain, GrayFace fix progress and the latest lane results.

## Summary

MM6 is a playtest candidate, not a released or campaign-complete title. The `ECv0.1.0` release ships an MM6 helper
behind the Atlas **Play MM6** button for owner playtests only. The planned MM6 release is `ECv0.2.0`, date undecided.
Since 2026-11-18 finished MM6 lanes merge into the long-lived `claude/mm6` branch, not `master`, until that tag.

The owner's delivery order puts MM6 working second, after MM7 working (closed 2026-11-11). "Working" means a separate
MM6 runtime and profile support New Game or Load, visible play, travel and main campaign progression through an ending
with durable saves, while MM7 keeps working. Headless tests alone never close that gate.

## What a player can do today

Every item below has input-driven game tests against the owner's real MM6 data. Suites named `...Headless` and
`DISABLED_` cases run in fresh processes through the gate runners.

- **New Game.** Party creation by input (portraits, voices, names, classes, skills, attributes) and production New
  Game into New Sorpigal. `MM6PartyCreation.*`, `MM6Creation.*`, `MM6NewGame.*`.
- **HUD and books.** MM6 HUD, character screen and paperdoll, spellbook, quest, autonotes, map and calendar books,
  awards. `MM6Hud.*`, `MM6BooksHeadless.*`, `MM6CharacterScreenHeadless.*`.
- **Houses.** Every New Sorpigal house kind opens with its MM6 layout. House exits respect their quest bits.
  `MM6Houses.*`, `MM6HouseExit.*`.
- **Shops.** Buy, sell, identify, repair and special stock. A character who can't act is refused, and so is a
  zero-value item. `MM6Houses.WeaponShop*`, `MM6Houses.GeneralStore*`.
- **Services.** Temple heal and donate, training, spell and skill guilds with membership, expert and master teachers,
  bank deposit and withdraw, tavern rent, fill packs, drink and tip, the town hall Bounty Hunt. One `MM6Houses` test
  per service.
- **NPCs.** Street NPC talk with weekday, news and Join topics, Beg, Threat and Bribe, hiring two followers, house
  NPC topics. `MM6NpcDialogue.*`, `MM6StreetNpcs.*`, `MM6Hirelings.*`.
- **Combat.** Melee, bows (Percival arrows explode), admitted player spells, monster missiles, spells and special
  attacks, trap and event damage, turn-based mode, potions, rest and wait. `MM6Melee.*`, `MM6Bow.*`,
  `MM6PlayerSpells.*`, `MM6TurnBased.*`, `MM6Potion.*`, `MM6Rest.*`, `MM6CombatHeadless.*`.
- **Death.** A party that dies in a dungeon or outdoors wakes in New Sorpigal by MM6 rules. `MM6PartyDeath.*`.
- **Dungeons.** Goblinwatch (d01), the Abandoned Temple (d02) and Gharik's Forge (d18): doors, switches, chests, traps
  and wall plates. `MM6Dungeons.*`, `MM6WallPlate.*`, `MM6Chest*`.
- **Travel.** Walking off a map edge, dungeon doors and back, stable and boat routes including the price-fixing quest,
  map state kept across a return, map timers that count while the party is away. `MM6MapTravel.*`, `MM6Transport.*`,
  `MM6MapVisit.*`.
- **Save and load.** Save on any map in the EC6 container, autosave on map change, quick save, in-game Load and load
  in a fresh process. `MM6InGameSave.*`, `MM6InGameLoad.*`, `MM6SaveAnywhereHeadless.*`, `MM6AutosaveHeadless.*`.
- **Finale.** The hive is won with the ritual and lost on the scripted failure paths. `MM6Finale.*`.

Spells MM6 has not admitted refuse with a status line instead of running inherited MM7 code. The admission list is
[MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h). Town Portal, Lloyd's Beacon, Stun, Shield, scrolls
and wands are not admitted. Rules come from the original `MM6.exe` in the data folder through `mm6::Executable`, see
[MM6 executable rules](mm6-executable-rules.md).

## How it is verified

| Method | What it proves | Where |
| --- | --- | --- |
| Game tests | Input-driven scenarios against the real data, observed through tapes. Each fix is reverted once to see its test fail. | `GameTests_MM6*.cpp`, `--gtest_filter='MM6*'` |
| Unit tests | Parsers, rule tables, save codecs. Data-backed cases read the install named by `OPENENROTH_MM6_PATH` and skip without it. | `MM6*_ut.cpp`, `test/Testing/Unit/MM6TestData.*` |
| MM7 controls | A shared-code change keeps MM7 working | `Prs.Pr2354`, `Prs.Pr2956`, `Issues.Issue840` plus the MM7 tests on the same path |
| First-map gate | New Game, save, fresh-process load, autosave, quick save and buffs across processes, each child in its own temporary profile | `tools/run_mm6_firstmap_gate.py` |
| Sweeps | Every MM6 map loads and runs its on-load events with actors. Monster combat on main-quest maps. | `tools/run_mm6_map_sweep.py`, `tools/run_mm6_combat_sweep.py` |
| Campaign chain | Each chapter loads the previous chapter's save in a fresh process | `tools/run_mm7_campaign_chain.py --title mm6` |
| Hidden captures | Before and after frames of each gameplay lane, rendered offscreen and inspected whole | temporary `MM6OffscreenFrame.DISABLED_*` tests, PNGs in the ignored `local-game-data/mm6/` |
| Owner playtests | Visible play of the installed helper | 2026-11-15 short playtest, 2026-11-17 playtest and tour |

The last map sweep on record loaded all 67 MM6 maps and ran every phase on all 26 main-quest maps without a crash.
`check_tidy` needs clang-tidy 22, which the owner's Mac lacks, so MM6 lanes run `check_style` only.

## Working-gate rows

| Row | What closes it | State |
| --- | --- | --- |
| M1 | Production New Game, Save, fresh-process Load | Done (first-map gate) |
| M2 | Live travel, Save on any map, fresh-process Load restoring chests, dead monsters and doors on active and parked maps | Done headless |
| M3 | Party death and respawn, in-game Load, autosave and quick save | Done headless |
| M4 | Main-quest chain from New Game to the finale with fresh-process checkpoints | C0 to C6 merged, C7 to C9 open |
| M5 | Owner sessions: the first hours, travel and a dungeon, chain checkpoints and the ending | Partial, New Sorpigal playtests only |
| M6 | The Atlas installs a sibling MM6 helper with its own profile, MM7 untouched | Helper ships in `ECv0.1.0` on `Profiles/MM6-Atlas` |
| M7 | MM7 stays working after every shared-core merge | Continuous |
| M8 | Player-blocking defects from M1 to M7 fixed | Ongoing |

## Campaign chain

The main quest spans 26 maps plus `global.evt`. The chain splits it into chapters C0 to C9. Each chapter is a
`MM6CampaignChain.DISABLED_ChapterN` game test that loads the previous chapter's `saveNNN.ec6` in a fresh process,
plays its part by input and writes the next save. Fixtures are named per chapter: C1's party strength and teleports
in front of doors, crystals and exits.

| Chapter | Content | State |
| --- | --- | --- |
| C0 | New Game and the opening | Merged |
| C1 | Councils A and C, stable pricing, a chest pickup | Merged |
| C2 | Council B | Merged |
| C3 | Councils D and E | Merged |
| C4 | Council F and the traitor | Merged |
| C5 | Oracle admission and the t6 crystal | Merged |
| C6 | The three castle crystals (Alamos, Darkmoor, Kriegspire) and the Oracle hand-in | Merged |
| C7 | The Tomb of VARN and the Control Cube | Not written. A draft fails on the real C6 save, see below |
| C8 | The NPC 12 release, through the room chain between houses 168, 553 and 554 | Not written |
| C9 | The hive finale | Not written as a chapter. `MM6Finale.*` covers the hive itself. |

The chapter helpers live in [MM6CampaignChain.h](../test/Bin/GameTest/MM6CampaignChain.h). Private event traces for
each chapter stay in the ignored `local-game-data/private-wiki/mm6/`.

### C7 route and what blocks it

A C7 draft from 2026-11-12 passed only on a stand-in for C6. On the real C6 save (2026-11-19) it fails in the Tomb of
VARN. The route it found:

- Starts on `outc2`. The Oracle's screen (`oracle.blv` door 2) is already raised from C5 and her floor plate (event 2)
  toggles it, so the party walks up without the plate. Face event 3, NPC 8, topic 76: award 33, quest bit 166, bits 162
  to 165 cleared. Out through event 50.
- `outb3` event 90 leads into `pyramid.blv`. The keyed doors, chests and pools check the active character's items, so
  one character has to carry every key and code and be active at each check.
- Switch event 29 opens doors 37 and 38 over chest 44 (keys 574, 537, 466, bit 231). Flame Door event 31 takes 574.
  Chest 42 holds 573 and 538 (bit 229), the Water Temple door (event 11) takes 573. Chest 45 holds 576 and 539 (bit 232).
  Chest 43 takes 576 and holds 577 and 542 (bit 230). Bookshelves 26 and 27 give 540 (bit 234) and 541 (bit 233).
- Each code is spoken holding its scroll: pools 21 to 25 take `kcops`, `ulus`, `aruhu`, `yttocs` and `yoccm`, the Well of
  VARN (event 1) takes `krik` and shows "The waters part.". The well is two pressure plates side by side, faces 226 and
  3293, and `walkOntoMM6CampaignChainIndoorEventPlate` stops on either.
- Control Room Entry event 35, then chest 41 takes 577 and gives the Control Cube 456 (bits 42 and 195). Out through
  event 17. The Oracle's topic 76 takes the cube for award 34 and 500,000 experience each, topic 77 sets bits 169 and 235
  and moves Nicolai from topic 25 to 26.

What blocks it: a Djinn and two Genies (actors 35 to 37) stand at switch 29 and fight the party there. Their hits keep
the party in recovery and move the active character, and the first character is asleep by chest 43, which then
answers "The chest is locked". With `NoDamage` and C1's party strength the party hits the Djinn (325 HP, level 44,
armor class 45, physical resistance 30) for 2 to 18 and is mostly unable to act, so `killMM6CampaignChainActor` gives up
after 500 attacks. C7 needs a way past them that a player would take, and the key carrier selected again before each
keyed check. Kill experience then joins the Oracle's 2,000,000.

## GrayFace fix progress

On 2026-11-15 the owner decided MM6 fixes every bug the GrayFace MM6 Patch 2.5.7 fixes, GrayFace's way or a better
one. A fix that changes game logic sits behind a `gameplay.fix_*` flag that defaults on. Item numbers below are the
ones in [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md).

Landed:

| Items | How |
| --- | --- |
| C3, C5, C7, C17, C20, C21, C27, C28, C29, W3, W6, W11, W14, W16, W17, W24, W27 | Already right in code. Those without an MM6 test got one in `GameTests_MM6GrayFaceShared.cpp`. |
| C10, C13, C14 | `fix_condition_spells`, `fix_water_walk_mana_drain`, `fix_stronger_shorter_buff` |
| C6, C18 | `fix_recovery_enchant`, `fix_turn_based_turn_time` |
| C26 | No flag. MM6.exe plays the mace swing for clubs, the port was wrong. |
| W1, W7, W8, W9 | `fix_map_timers`, `fix_travel_autosave`, `fix_travel_speech_delay`, `fix_flying_border_offer` |
| W2 | `fix_inactive_party_events` |
| W4, W5, W18 | `fix_new_day_at_month_start`, `fix_sunrise_light`, `fix_buff_duration_text` |
| W22, W26 | `fix_wasted_black_potion`, `fix_shops_buying_blasters` |
| C12 | `fix_spells_cost` |
| W28 | No flag. MM6.exe already refused paid shop services. The skill half waits for the shop item info box. |

Open:

| Items | What is left |
| --- | --- |
| C8 | Weapon-skill hirelings add to an MM7 skill MM6 doesn't have. Trace what vanilla applies the bonus to first. |
| C22 | Armor recovery uses the MM7 table. Mok's change needs tracing. |
| C24 | Doesn't reproduce. MM6.exe's own walking edge check, which we port, keeps chasing monsters on their ledges. Needs a vanilla case that falls. |
| C25 | The item roll still favours the first candidate |
| W12 | Found-artifact flags never clear on respawn. Needs event opcode 69 `RefundChestArtifacts`. |
| W13 | Chest overflow items stay hidden forever |
| W15 | Door action 3 (switch while moving) is ignored |
| W19 | The stats screen shows the display condition, not the one that sets the stats |
| C2, C4, C9, C11, C19, W10, W20, W21 | Fixed when the spell or feature is admitted, not before |
| Data fixes | Script fixes as an engine-side patch table keyed by file hash. Map fixes through `*.games.lod` override archives. |

## Latest lane results

The lanes below merged into `claude/mm6` on 2026-11-18, the travel lane at `a927bd004` the next morning. Each ran the
full `MM6*` game suite, the unit tests and the MM7 controls, and took before and after captures.

- **shop-unconscious (W28).** MM6 shops refuse identify, repair and every paid screen to a character who can't act,
  with MM6's "in no condition" line. Left: shops have no right-click item info box yet.
- **shop-menu.** Shop, tavern and training menus hide their options from a character who can't act, as MM6.exe does.
  Left: no tavern-specific test.
- **buff-compare (C14).** A stronger buff replaces a longer weaker one. Left: pressing I draws the stats tab for one
  frame before the inventory.
- **outdoor-actor-alpha.** Outdoor MM6 actors draw opaque. MM6 monster records have no tint field, and a filled-in
  opaque white tint sent them down the additive blend path. This was the owner's "transparent villagers".
- **wall-plates.** Pressure plates on walls and ceilings fire when the party runs into them, as in MM6.exe. MM7 keeps
  upstream's behaviour, which drops them.
- **street-npc-talk.** Beg, Threat and Bribe with MM6's acceptance and price rules, the `%02` and `%08` text codes,
  Join as a sub-screen. Left: speeches 53 to 58 show only as portrait reactions.
- **street-npc-followups.** Bribe count, tavern drink bits and tavern tips are saved in EC6 `PART` v5. Street NPCs are
  cleared on New Game. Left: news topics carry over across map loads within one game, as in vanilla.
- **event-member (W2).** An event triggered with nobody active runs for one member who can act. Left: other
  active-member reads in the same run, such as held-item moves, still see nobody active.
- **offline-fixtures.** Data-backed unit tests read the MM6 install directly through `MM6TestData`. The Python
  research scripts are gone. Left: a few `GameTests_MM6Save` cases still need the private research folder.
- **grayface-items (W22, W26).** A repeated Essence stays in the hand. Shops refuse zero-value items such as blasters.
  Left: the MM6 sell screen plays no sale or refusal speech and sound.
- **rules-from-exe.** Rule tables come from `MM6.exe` through `mm6::Executable` with a SHA-256 check. `--rules-path` and
  the profile rules JSON are gone. Adds `fix_condition_spells`. Left: C12.
- **grayface-combat (C6, C18, C26).** The recovery enchantment shortens recovery by 10 percent, a turn-based round takes
  50 game seconds, clubs and empty hands swing with the mace sound.
- **shared-coverage.** MM6 tests for ten GrayFace fixes in shared code, and fixes for month-start New Day, 5:00 AM
  sunrise light and minutes next to days in buff times. Left: MM7 has the same 5:00 AM sunlight code.
- **grayface-travel (W1, W7, W8, W9).** No map-crossing offer while flying, the travel autosave is written before the
  fare, travel leaves when its speech ends, map timers count from their last firing (new optional EC6 `TIMR`
  section). Left: no travel speech plays yet, so the W8 cap always applies.
- **travel-dialog-icons.** The map-edge travel dialog draws MM6.exe's own layout, `evpan004`, the `outside` picture and
  `BUTTYES1` and `BUTTESC1` at y 313, instead of MM7 icons MM6 lacks.
- **dungeon-dialog.** The dungeon entry and exit dialog draws MM6.exe's layout too: the room's `evpan` panel, the exit
  picture at (525,34), the buttons at y 313 and the hour's top panel beside it. The exit dialog names the dungeon, and
  the Dragon's Lair and Devil Outpost doors show their `trans.txt` lines. Left: the sound and speech calls MM6.exe makes
  when the dialog opens, see [Open questions](open-questions.md).
- **docs.** `project/`, `.llm-wiki/` and the tracked `agent_comms/` notes became this `docs/` wiki.
- **code-cleanup.** MM6 code only its own tests reached, the save-research probe tests and the research-folder map
  reader are gone. No MM6 test skips on a machine with the MM6 data. Left: files such as `MM6NewGameSaveRuntime.cpp`
  are live but mostly dead and want a finer pass.

The merged branch passed 298 `MM6*` game tests with none skipped, 1084 unit tests, `check_style`, the first-map gate
and 729 of 730 MM7 game tests. The one MM7 failure, `Issues.Issue2817`, fails the same way on `master`. A 41-frame
hidden tour on the same build showed every outdoor actor opaque and every change above on screen.

Earlier MM6 lanes (menus and hirelings, services, rest and time rules) reached `master` before `ECv0.1.0`.

## Source

- [MM6Executable.h](../src/Engine/MM6/MM6Executable.h)
- [MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h)
- [GameConfig.h](../src/Application/GameConfig.h), the `gameplay.fix_*` flags
- [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp), [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp)
- [MM6CampaignChain.cpp](../test/Bin/GameTest/MM6CampaignChain.cpp),
  [GameTests_MM6CampaignChainC6.cpp](../test/Bin/GameTest/GameTests_MM6CampaignChainC6.cpp)
- [GameTests_MM6GrayFaceShared.cpp](../test/Bin/GameTest/GameTests_MM6GrayFaceShared.cpp),
  [GameTests_MM6Finale.cpp](../test/Bin/GameTest/GameTests_MM6Finale.cpp)
- [run_mm6_firstmap_gate.py](../tools/run_mm6_firstmap_gate.py), [run_mm6_map_sweep.py](../tools/run_mm6_map_sweep.py),
  [run_mm6_combat_sweep.py](../tools/run_mm6_combat_sweep.py), [run_mm7_campaign_chain.py](../tools/run_mm7_campaign_chain.py)

See also: [Open questions](open-questions.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md), [Testing](testing.md), [Releases](releases.md)
